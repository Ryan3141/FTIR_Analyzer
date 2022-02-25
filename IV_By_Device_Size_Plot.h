#pragma once
#include "qcustomplot.h"

#include <vector>
#include <functional>
#include <armadillo>

#include <QString>
#include "Handy_Types_And_Conversions.h"

#include "Interactive_Graph.h"


namespace Report
{

struct Axes
{
	inline std::tuple< QVector<double>, QVector<double> > Prepare_XY_Data( const Single_Graph< X_Units, Y_Units > & graph )
	{
		arma::vec x_data = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
		arma::vec y_data = arma::conv_to<arma::vec>::from( graph.y_data.toStdVector() );
		switch( this->y_units )
		{
		case Y_Units::CURRENT_A:
		break;
		case Y_Units::CURRENT_A_PER_AREA_CM:
		{
			double side_cm = graph.meta.find( "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)" )->second.toDouble() * 1E-4;
			y_data /= side_cm * side_cm;
		}
		break;
		case Y_Units::LOG_CURRENT_A:
		{
			//y_data = arma::log10( arma::abs( y_data ) );
			y_data = arma::abs( y_data );
		}
		break;
		case Y_Units::LOG_CURRENT_A_PER_AREA_CM:
		{
			double side_cm = graph.meta.find( "Device Side Length (" + QString( QChar( 0x03BC ) ) + "m)" )->second.toDouble() * 1E-4;
			//y_data = arma::log10( arma::abs( y_data / ( side_cm * side_cm ) ) );
			y_data = arma::abs( y_data / ( side_cm * side_cm ) );
		}
		break;
		}
		return { toQVec( x_data ), toQVec( y_data ) };
	}

	Axes( std::function<void()> regraph_function );

	const static X_Units default_x_units = X_Units::AREA_OVER_PERIMETER_M;
	const static Y_Units default_y_units = Y_Units::CURRENT_A;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	const static QString X_Unit_Names[ 1 ];
	const static QString Y_Unit_Names[ 4 ];
	const static QString Change_To_Y_Unit_Names[ 4 ];
};

class Interactive_Graph :
	public ::Interactive_Graph<X_Units, Y_Units, Axes>
{
public:
	Interactive_Graph( QWidget *parent = nullptr );

	template< X_Units X, Y_Units Y >
	const Single_Graph< X_Units, Y_Units > & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title, Labeled_Metadata meta )
	{
		//const Single_Graph< X_Units, Y_Units > & x = ::Interactive_Graph<X_Units, Y_Units, Axes>::Graph<X, Y>( x_data, y_data, measurement_name, graph_title, meta );
		if( x_data.size() != y_data.size() )
		{
			std::cerr << "Incompatible sizes being graphed, size(x) = " << x_data.size() << " size(y) = " << y_data.size() << "\n";
			if( x_data.size() > y_data.size() )
				x_data.resize( y_data.size() );
			else
				y_data.resize( x_data.size() );
		}

		auto existing_graph = remembered_graphs.find( measurement_name );
		if( existing_graph != remembered_graphs.end() )
		{
			auto & current_info = existing_graph->second;
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
			remembered_graphs[ measurement_name ] = Single_Graph< X_Units, Y_Units >{ X, Y, std::move( x_data ), std::move( y_data ), current_graph, meta, std::nullopt };
			graphs_in_order.push_back( &remembered_graphs[ measurement_name ] );
			auto[ adjusted_x_data, adjusted_y_data ] = axes.Prepare_XY_Data( remembered_graphs[ measurement_name ] );
			this->graph()->setData( adjusted_x_data, adjusted_y_data );
			this->graph()->setLineStyle( QCPGraph::lsNone );
			this->graph()->setScatterStyle( QCPScatterStyle::ssCircle );
			QPen graphPen;
			//QCPColorGradient gradient( QCPColorGradient::gpPolar );
			QCPColorGradient gradient( QCPColorGradient::gpSpectrum );
			gradient.setPeriodic( true );
			graphPen.setColor( gradient.color( color_index / 10.0, QCPRange( 0.0, 1.0 ) ) );
			//graphPen.setWidthF( 2 ); Changing width currently causes massive performance issues
			color_index++;
			this->graph()->setPen( graphPen );

			return remembered_graphs[ measurement_name ];
		}
	}


	//const Single_Graph< X_Units, Y_Units > & Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title = QString(), Labeled_Metadata meta = Labeled_Metadata{} );

private:
	QSharedPointer<QCPAxisTicker> linearTicker;
	QSharedPointer<QCPAxisTickerLog> logTicker;
};

}
