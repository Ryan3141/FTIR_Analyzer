#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <map>

#include <QVector>

#include "Units.h"
#include "Handy_Types_And_Conversions.h"


struct Single_Graph
{
	X_Unit_Type x_units;
	Y_Unit_Type y_units;
	QVector<double> x_data;
	QVector<double> y_data;
	QCPGraph* graph_pointer;
	Labeled_Metadata meta;
};


struct Axes_Scales
{
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;

	X_Unit_Type x_units = X_Unit_Type::WAVELENGTH_MICRONS;
	Y_Unit_Type y_units = Y_Unit_Type::RAW_SENSOR;
	std::function<void()> graph_function;

	Axes_Scales( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	void Set_X_Units( X_Unit_Type units );
	void Set_Y_Units( Y_Unit_Type units );

	void Set_As_Background( XY_Data xy );

	std::tuple< QVector<double>, QVector<double> > Prepare_XY_Data( const Single_Graph & graph_data ) const;

	QVector<double> background_x_data;
	QVector<double> background_y_data;
};



class Interactive_Graph :
	public QCustomPlot
{
	Q_OBJECT


signals:
	void Graph_Selected( QCPGraph* selected_graph );
	void X_Units_Changed( X_Unit_Type new_units );
	void Y_Units_Changed( Y_Unit_Type new_units );
public:
	explicit Interactive_Graph( QWidget *parent = 0 );
	void Initialize_Graph();

	void selectionChanged();
	void mousePress();
	void mouseDoubleClickEvent( QMouseEvent* event ) override;
	void titleDoubleClick( QMouseEvent * event );
	void axisLabelDoubleClick( QCPAxis * axis, QCPAxis::SelectablePart part, QMouseEvent * event );
	void legendDoubleClick( QCPLegend * legend, QCPAbstractLegendItem * item, QMouseEvent * event );
	void mouseWheel();
	void refitGraphs( QMouseEvent* event );
	void removeSelectedGraph();
	void removeAllGraphs();
	void graphContextMenuRequest( QPoint pos );
	void moveLegend();
	void graphClicked( QCPAbstractPlottable *plottable, int dataIndex );
	void saveCurrentGraph();
	void Hide_Graph( QString graph_name );
	void RegraphAll();
	void UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data );
	const Single_Graph & GetSelectedGraphData() const;
	const Single_Graph & FindDataFromGraphPointer( QCPGraph* graph_pointer ) const;

	template< X_Unit_Type X_Units, Y_Unit_Type Y_Units >
	const Single_Graph & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title = QString(), Labeled_Metadata meta = Labeled_Metadata() )
	{
		if( x_data.size() != y_data.size() )
		{
			std::cerr << "Incompatible sizes being graphed, size(x) = " << x_data.size() << " size(y) = " << y_data.size() << "\n";
			if( x_data.size() > y_data.size() )
				x_data.resize( y_data.size() );
			else
				y_data.resize( x_data.size() );
		}

		for( auto & x : x_data )
			if( !std::isnormal( x ) )
				x = qQNaN();
		for( auto & y : y_data )
			if( !std::isfinite( y ) || y > 1.0E4 )
				y = qQNaN();

		auto existing_graph = remembered_graphs.find( measurement_name );
		if( existing_graph != remembered_graphs.end() )
		{
			Single_Graph & current_info = existing_graph->second;
			//if( current_info.x_data.size() == x_data.size() &&
			//	current_info.y_data.size() == y_data.size() )
			{
				current_info.x_data = x_data;
				current_info.y_data = y_data;
				auto[ adjusted_x_data, adjusted_y_data ] = axes.Prepare_XY_Data( current_info );
				current_info.graph_pointer->setData( adjusted_x_data, adjusted_y_data );
				current_info.graph_pointer->setVisible( true );
				current_info.graph_pointer->addToLegend();

			}

			if( !meta.empty() )
				current_info.meta = meta;
			//this->replot();
			return current_info;
		}

		// Add graph
		{
			//double previous_upper_limit = this->yAxis->range().upper;
			static int color_index = 0;
			bool this_is_the_first_graph = this->graphCount() == 0;
			QCPGraph* current_graph = this->addGraph();

			if( graph_title.isEmpty() )
				current_graph->setName( measurement_name );
			else
				current_graph->setName( graph_title );

			// Remember data before changing it at all
			remembered_graphs[ measurement_name ] = Single_Graph{ X_Units, Y_Units, std::move( x_data ), std::move( y_data ), current_graph, meta };
			auto[ adjusted_x_data, adjusted_y_data ] = axes.Prepare_XY_Data( remembered_graphs[ measurement_name ] );
			this->graph()->setData( adjusted_x_data, adjusted_y_data );
			this->graph()->setLineStyle( QCPGraph::lsLine );// (QCPGraph::LineStyle)(rand() % 5 + 1) );
																	 //if( rand() % 100 > 50 )
																	 //	this->graph()->setScatterStyle( QCPScatterStyle( (QCPScatterStyle::ScatterShape)(rand() % 14 + 1) ) );
			QPen graphPen;
			//QCPColorGradient gradient( QCPColorGradient::gpPolar );
			QCPColorGradient gradient( QCPColorGradient::gpSpectrum );
			gradient.setPeriodic( true );
			//gradient.setLevelCount( 10 );
			const QVector< Qt::PenStyle > patterns = { Qt::SolidLine, Qt::DotLine, Qt::DashLine, Qt::DashDotDotLine, Qt::DashDotLine };
			graphPen.setColor( gradient.color( color_index / 10.0, QCPRange( 0.0, 1.0 ) ) );
			graphPen.setStyle( patterns[ color_index % patterns.size() ] );
			//graphPen.setWidthF( 2 ); Changing width currently causes massive performance issues
			color_index++;
			//graphPen.setColor( QColor::fromHslF( (color_index++)/10.0*0.8, 0.95, 0.5) );
			//graphPen.setColor( QColor::fromHsv( rand() % 255, 255, 255 ) );
			//graphPen.setWidthF( rand() / (double)RAND_MAX * 2 + 1 );
			this->graph()->setPen( graphPen );
			//if( this_is_the_first_graph )
			//{
			//	//this->rescaleAxes();
			//	this->yAxis->rescale();
			//	this->yAxis->setRangeLower( 0 );
			//	double upper = this->yAxis->range().upper;
			//	//this->yAxis->setRangeUpper( std::max( previous_upper_limit, std::min( upper, 1. ) ) );
			//	if( allow_y_scaling )
			//		this->yAxis->setRangeUpper( std::min( upper, 100. ) );
			//	else
			//		this->yAxis->setRangeUpper( upper );
			//}
			//this->yAxis->setRange( -10, 110 );
			//this->replot();

			return remembered_graphs[ measurement_name ];
		}
	}

	Axes_Scales axes;

private:
	std::map< QString, Single_Graph > remembered_graphs;
	std::vector< std::function<void( Interactive_Graph*, QMenu*, QPoint )> > x_axis_menu_functions;
	std::vector< std::function<void( Interactive_Graph*, QMenu*, QPoint )> > y_axis_menu_functions;
};

std::tuple< std::vector<double>, std::vector<double> > Load_XY_CSV_Data( const std::string & data_to_parse, const char* delimiter = "," );
