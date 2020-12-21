#include "Interactive_Graph.h"

#include <array>
#include <algorithm>
#include <armadillo>


#include "boost/algorithm/string.hpp"
#include "fn.hpp"
namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));


static QString Unit_Names[ 3 ] = { "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
							"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
							"Photon Energy (eV)" };

Interactive_Graph::Interactive_Graph( QWidget *parent ) :
	QCustomPlot( parent )
{
	Initialize_Graph();
}

void Interactive_Graph::Initialize_Graph()
{
	this->setOpenGl( true );
	this->setInteractions( QCP::iRangeDrag | QCP::iRangeZoom | QCP::iSelectAxes |
									QCP::iSelectLegend | QCP::iSelectPlottables );
	this->xAxis->setRange( 1, 10 );
	this->yAxis->setRange( 0, 100 );
	this->axisRect()->setupFullAxesBox();

	//this->plotLayout()->insertRow( 0 );
	//QCPTextElement *title = new QCPTextElement( this, "Transmission", QFont( "sans", 17, QFont::Bold ) );
	//this->plotLayout()->addElement( 0, 0, title );

	this->xAxis->setLabel( Unit_Names[ int(Unit_Type::WAVELENGTH_MICRONS) ] );
	this->yAxis->setLabel( "Transmission (%)" );
	this->legend->setVisible( true );
	{ // Set up fonts
		QFont legendFont = font();
		legendFont.setPointSize( 10 );
		this->legend->setFont( legendFont );
		this->legend->setSelectedFont( legendFont );
		this->xAxis->setLabelFont( legendFont );
		this->yAxis->setLabelFont( legendFont );
		this->xAxis->setTickLabelFont( legendFont );
		this->yAxis->setTickLabelFont( legendFont );
	}
	this->legend->setSelectableParts( QCPLegend::spItems ); // legend box shall not be selectable, only legend items
	this->setAntialiasedElements( QCP::aeAll );

																	 // connect slot that ties some axis selections together (especially opposite axes):
	connect( this, &QWidget::customContextMenuRequested, this, &Interactive_Graph::graphContextMenuRequest );
	connect( this, &QCustomPlot::selectionChangedByUser, this, &Interactive_Graph::selectionChanged );
	// connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
	connect( this, &QCustomPlot::mousePress, this, &Interactive_Graph::mousePress );
	connect( this, &QCustomPlot::mouseWheel, this, &Interactive_Graph::mouseWheel );

	connect( this, &QCustomPlot::mouseDoubleClick, this, &Interactive_Graph::refitGraphs );

	// make bottom and left axes transfer their ranges to top and right axes:
	connect( this->xAxis, SIGNAL( rangeChanged( QCPRange ) ), this->xAxis2, SLOT( setRange( QCPRange ) ) );
	connect( this->yAxis, SIGNAL( rangeChanged( QCPRange ) ), this->yAxis2, SLOT( setRange( QCPRange ) ) );

	// connect some interaction slots:
	connect( this, &QCustomPlot::axisDoubleClick, this, &Interactive_Graph::axisLabelDoubleClick );
	connect( this, &QCustomPlot::legendDoubleClick, this, &Interactive_Graph::legendDoubleClick );
	//connect( title, &QCPTextElement::doubleClicked, this, &Interactive_Graph::titleDoubleClick );

	//// connect slot that shows a message in the status bar when a graph is clicked:
	//connect( this, SIGNAL( plottableClick( QCPAbstractPlottable*, int, QMouseEvent* ) ), this, SLOT( graphClicked( QCPAbstractPlottable*, int ) ) );

	// setup policy and connect slot for context menu popup:
	this->setContextMenuPolicy( Qt::CustomContextMenu );

	this->x_axis_menu_functions.push_back( [this]( Interactive_Graph* graph, QMenu* menu, QPoint pos )
	{
		auto Fix_X_Range = [graph, this]( Unit_Type new_type )
		{
			std::array<double, 2> bounds = { xAxis->range().lower, xAxis->range().upper };
			for( double & x : bounds )
				x = Convert_Units( this->x_axis_units, new_type, x );
			xAxis->setRange( std::min( bounds[ 0 ], bounds[ 1 ] ), std::max( bounds[ 0 ], bounds[ 1 ] ) );
			this->x_axis_units = new_type;
			graph->xAxis->setLabel( Unit_Names[ int( new_type ) ] );
			emit X_Units_Changed( new_type );
		};
		menu->addAction( "Change to Wavelength", [this, Fix_X_Range]
		{
			Fix_X_Range( Unit_Type::WAVELENGTH_MICRONS );
			this->RegraphAll();
		} );
		menu->addAction( "Change to Wave Number", [this, Fix_X_Range]
		{
			Fix_X_Range( Unit_Type::WAVE_NUMBER );
			this->RegraphAll();
		} );
		menu->addAction( "Change to Energy", [this, Fix_X_Range]
		{
			Fix_X_Range( Unit_Type::ENERGY_EV );
			this->RegraphAll();
		} );
	} );
}

void Interactive_Graph::mouseDoubleClickEvent( QMouseEvent* event )
{
	QCustomPlot::mouseDoubleClickEvent( event );

	if( !event->isAccepted() )
		this->refitGraphs( event );
}

void Interactive_Graph::titleDoubleClick( QMouseEvent* event )
{
	Q_UNUSED( event )
		if( QCPTextElement *title = qobject_cast<QCPTextElement*>(sender()) )
		{
			event->accept();
			// Set the plot title by double clicking on it
			bool ok;
			QString newTitle = QInputDialog::getText( this, "Change Title", "New plot title:", QLineEdit::Normal, title->text(), &ok );
			if( ok )
			{
				title->setText( newTitle );
				this->replot();
			}
		}
}

void Interactive_Graph::axisLabelDoubleClick( QCPAxis *axis, QCPAxis::SelectablePart part, QMouseEvent * event )
{
	// Set an axis label by double clicking on it
	if( part == QCPAxis::spAxisLabel ) // only react when the actual axis label is clicked, not tick label or axis backbone
	{
		event->accept();
		bool ok;
		QString newLabel = QInputDialog::getText( this, "Change Axis Label", "New axis label:", QLineEdit::Normal, axis->label(), &ok );
		if( ok )
		{
			axis->setLabel( newLabel );
			this->replot();
		}
	}
}

void Interactive_Graph::legendDoubleClick( QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent * event )
{
	// Rename a graph by double clicking on its legend item
	Q_UNUSED( legend )
		if( item ) // only react if item was clicked (user could have clicked on border padding of legend where there is no item, then item is 0)
		{
			event->accept();
			QCPPlottableLegendItem *plItem = qobject_cast<QCPPlottableLegendItem*>(item);
			bool ok;
			QString newName = QInputDialog::getText( this, "Change Graph Name", "New graph name:", QLineEdit::Normal, plItem->plottable()->name(), &ok );
			if( ok )
			{
				plItem->plottable()->setName( newName );
				this->replot();
			}
		}
}

void Interactive_Graph::selectionChanged()
{
	/*
	normally, axis base line, axis tick labels and axis labels are selectable separately, but we want
	the user only to be able to select the axis as a whole, so we tie the selected states of the tick labels
	and the axis base line together. However, the axis label shall be selectable individually.

	The selection state of the left and right axes shall be synchronized as well as the state of the
	bottom and top axes.

	Further, we want to synchronize the selection of the graphs with the selection state of the respective
	legend item belonging to that graph. So the user can select a graph by either clicking on the graph itself
	or on its legend item.
	*/

	// make top and bottom axes be selected synchronously, and handle axis and tick labels as one selectable object:
	if( this->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) || this->xAxis->selectedParts().testFlag( QCPAxis::spTickLabels ) ||
		this->xAxis2->selectedParts().testFlag( QCPAxis::spAxis ) || this->xAxis2->selectedParts().testFlag( QCPAxis::spTickLabels ) )
	{
		this->xAxis2->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
		this->xAxis->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
	}
	// make left and right axes be selected synchronously, and handle axis and tick labels as one selectable object:
	if( this->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) || this->yAxis->selectedParts().testFlag( QCPAxis::spTickLabels ) ||
		this->yAxis2->selectedParts().testFlag( QCPAxis::spAxis ) || this->yAxis2->selectedParts().testFlag( QCPAxis::spTickLabels ) )
	{
		this->yAxis2->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
		this->yAxis->setSelectedParts( QCPAxis::spAxis | QCPAxis::spTickLabels );
	}

	// synchronize selection of graphs with selection of corresponding legend items:
	for( int i = 0; i < this->graphCount(); ++i )
	{
		QCPGraph *graph = this->graph( i );
		QCPPlottableLegendItem *item = this->legend->itemWithPlottable( graph );
		if( item != nullptr && (item->selected() || graph->selected()) )
		{
			item->setSelected( true );
			graph->setSelection( QCPDataSelection( graph->data()->dataRange() ) );
			emit Graph_Selected( graph );
		}
	}
}

void Interactive_Graph::mousePress()
{
	// if an axis is selected, only allow the direction of that axis to be dragged
	// if no axis is selected, both directions may be dragged

	if( this->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeDrag( this->xAxis->orientation() );
	else if( this->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeDrag( this->yAxis->orientation() );
	else
		this->axisRect()->setRangeDrag( Qt::Horizontal | Qt::Vertical );
}

void Interactive_Graph::mouseWheel()
{
	// if an axis is selected, only allow the direction of that axis to be zoomed
	// if no axis is selected, both directions may be zoomed

	if( this->xAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeZoom( this->xAxis->orientation() );
	else if( this->yAxis->selectedParts().testFlag( QCPAxis::spAxis ) )
		this->axisRect()->setRangeZoom( this->yAxis->orientation() );
	else
		this->axisRect()->setRangeZoom( Qt::Horizontal | Qt::Vertical );
}

void Interactive_Graph::refitGraphs( QMouseEvent* event )
{
	event->accept();
	this->rescaleAxes( true );
	this->yAxis->setRangeLower( 0 );
	//this->yAxis->setRange( -10, 110 );
	this->replot();
}

void Interactive_Graph::removeSelectedGraph()
{
	if( this->selectedGraphs().size() == 0 )
		return;

	QCPGraph* selected_graph = this->selectedGraphs().first();
	for( std::map< QString, Single_Graph >::iterator element = remembered_graphs.begin();
			element != remembered_graphs.end(); ++element )
	{
		if( element->second.graph_pointer == selected_graph )
		{
			remembered_graphs.erase( element );
			break;
		}
	}
	this->removeGraph( selected_graph );
	this->replot();
}

void Interactive_Graph::removeAllGraphs()
{
	this->clearGraphs();
	this->remembered_graphs.clear();
	this->replot();
}

void Interactive_Graph::graphContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );

	bool selected_legend = this->legend->visible() && this->legend->selectTest( pos, false ) >= 0;
	bool selected_x_axis = this->xAxis->selectTest( pos, false ) >= 0;
	bool selected_y_axis = this->yAxis->selectTest( pos, false ) >= 0;

	if( selected_legend ) // context menu on legend requested
	{
		menu->addAction( "Move to top left", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignLeft) );
		menu->addAction( "Move to top center", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignHCenter) );
		menu->addAction( "Move to top right", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignRight) );
		menu->addAction( "Move to bottom right", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignRight) );
		menu->addAction( "Move to bottom left", this, &Interactive_Graph::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignLeft) );
		menu->addAction( "Turn Legend Off", [this]
		{
			this->legend->setVisible( false );
			this->replot();
		} );
	}

	if( !selected_x_axis && !selected_y_axis && !selected_legend )
	{
		if( !this->legend->visible() )
			menu->addAction( "Turn Legend On", [this]
		{
			this->legend->setVisible( true );
			this->replot();
		} );
		if( this->selectedGraphs().size() > 0 )
			menu->addAction( "Remove selected graph", this, &Interactive_Graph::removeSelectedGraph );
		if( this->graphCount() > 0 )
			menu->addAction( "Remove all graphs", this, &Interactive_Graph::removeAllGraphs );
		menu->addAction( "Save current graph", this, &Interactive_Graph::saveCurrentGraph );
		menu->addAction( "Clear Background", this, &Interactive_Graph::Clear_Background );

		menu->addAction( "Paste From Clipboard", [this]
		{
			const QClipboard *clipboard = QApplication::clipboard();
			const QMimeData *mimeData = clipboard->mimeData();

			if( mimeData->hasText() )
			{
				auto [x_data, y_data] = Load_XY_CSV_Data( mimeData->text().toStdString(), ",\t" );
				this->Graph( toQVec( Convert_Units( Unit_Type::WAVELENGTH_MICRONS, Unit_Type::WAVE_NUMBER, arma::vec(x_data) ) ),
							 toQVec( arma::vec( y_data ) * 100.0 ), "Clipboard Data", "Clipboard Data" );
				this->replot();
			}
			else
			{
				//setText( tr( "Cannot display data" ) );
				return;
			}
			//std::vector< std::array<double,2> > test = fn::from( mimeData->text().toStdString() )
			//	% fn::group_adjacent_by( []( const char ch ) { return ch != '\n' && ch != '\r'; } )
			//	% fn::where( []( const auto ch ) { return ch != "\n" && ch != "\r"; } )
			//	% fn::transform( []( auto one_line )
			//		{
			//			return std::move( one_line ) % fn::group_adjacent_by( []( const char ch ) { return ch != ','; } )
			//				% fn::where( []( const auto ch ) { return ch != ","; } );
			//		} )
			//	% fn::transform( [this]( const auto & line_of_elements )
			//		{
			//			if( line_of_elements.size() >= 2 )
			//				return std::array<double, 2>{
			//					std::stod( line_of_elements[ 0 ] ),
			//					std::stod( line_of_elements[ 1 ] ) };
			//			else
			//				return std::array<double, 2>{};
			//		} )
			//	% fn::transform( [this]( const auto & line_of_elements )
			//		{
			//			if( line_of_elements.size() >= 2 )
			//				return std::array<double, 2>{
			//					std::stod( line_of_elements[ 0 ] ),
			//					std::stod( line_of_elements[ 1 ] ) };
			//			else
			//				return std::array<double, 2>{};
			//		} )
			//	% fn::to_vector();

			//using namespace ranges;
			////auto test = mimeData->text().toStdString() | views::split( '_' );
			//auto const s = std::string{ "feel_the_force" };
			//auto words = s | views::split('_'); // [[f,e,e,l],[t,h,e],[f,o,r,c,e]]
			//ui.customPlot->Graph( x_data, y_data, "Clipboard Data", "Clipboard Data" );
		} );
	}

	if( selected_x_axis )
		for( auto menu_function : this->x_axis_menu_functions )
			menu_function( this, menu, pos );

	menu->popup( this->mapToGlobal( pos ) );
}

void Interactive_Graph::moveLegend()
{
	if( QAction* contextAction = qobject_cast<QAction*>(sender()) ) // make sure this slot is really called by a context menu action, so it carries the data we need
	{
		bool ok;
		int dataInt = contextAction->data().toInt( &ok );
		if( ok )
		{
			this->axisRect()->insetLayout()->setInsetAlignment( 0, (Qt::Alignment)dataInt );
			this->replot();
		}
	}
}

void Interactive_Graph::graphClicked( QCPAbstractPlottable *plottable, int dataIndex )
{
	//// since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
	//// usually it's better to first check whether interface1D() returns non-zero, and only then use it.
	//double dataValue = plottable->interface1D()->dataMainValue( dataIndex );
	//QString message = QString( "Clicked on graph '%1' at data point #%2 with value %3." ).arg( plottable->name() ).arg( dataIndex ).arg( dataValue );
	//ui.statusBar->showMessage( message, 2500 );
}

const Single_Graph & Interactive_Graph::Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title, bool allow_y_scaling, Metadata meta )
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
		if( current_info.x_data.size() == x_data.size() &&
			current_info.y_data.size() == y_data.size() )
		{
			current_info.x_data = x_data;
			current_info.y_data = y_data;
			if( allow_y_scaling )
			{
				for( int i = 0; i < y_data.size(); i++ )
					y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
			}
			else
			{
			}
			for( double & x : x_data )
				x = Convert_Units( current_info.x_units, this->x_axis_units, x );
			current_info.graph_pointer->setData( x_data, y_data );
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
		//current_graph->
		if( graph_title.isEmpty() )
			current_graph->setName( measurement_name );
		else
			current_graph->setName( graph_title );

		//this->graph()->setName( meta_data.graph_title );
		// Remember data before changing it at all
		remembered_graphs[ measurement_name ] = Single_Graph{ Unit_Type::WAVE_NUMBER, x_data, y_data, current_graph, allow_y_scaling, meta };
		if( allow_y_scaling )
		{
			for( int i = 0; i < y_data.size(); i++ )
				y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
			//arma::vec test( y_data.size() );
			//for( int i = 0; i < y_data.size(); i++ )
			//{
			//	test( i ) = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
			//	y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
			//}
			//test = arma::real( arma::fft( test ) );
			//y_data = QVector<double>::fromStdVector( arma::conv_to<std::vector<double>>::from( test ) );
		}
		for( double & x : x_data )
			x = Convert_Units( remembered_graphs[ measurement_name ].x_units, this->x_axis_units, x );
		this->graph()->setData( x_data, y_data );
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

void Interactive_Graph::Hide_Graph( QString graph_name )
{
	auto existing_graph = remembered_graphs.find( graph_name );
	if( existing_graph == remembered_graphs.end() )
		return;

	Single_Graph & current_info = existing_graph->second;
	if( !current_info.graph_pointer->visible() ) // Already invisible, nothing to do
		return;
	current_info.graph_pointer->setVisible( false );
	current_info.graph_pointer->removeFromLegend();
	replot();
}

void Interactive_Graph::RegraphAll()
{
	//for( int i = 0; i < ui.customPlot->graphCount(); i++ )
	for( const auto &[ name, graph ] : remembered_graphs )
	{
		//QCPGraph* graph = ui.customPlot->graph( i );
		//QString measurement_id = graph->name();

		//QVector<double> x_data, y_data;
		//Grab_SQL_Data_From_Measurement_ID( measurement_id, x_data, y_data );

		QVector<double> x_data{ graph.x_data };
		QVector<double> y_data{ graph.y_data };
		if( graph.allow_y_scaling )
		{
			for( int i = 0; i < y_data.size(); i++ )
				y_data[ i ] = y_display_method( x_data[ i ], y_data[ i ] ); // Do the y stuff first to not screw up x data for y_display_method
		}
		for( double & x : x_data )
			x = Convert_Units( graph.x_units, this->x_axis_units, x );

		if( x_data.size() != y_data.size() )
			throw "Trying to graph different x and y size arrays";

		this->UpdateGraph( graph.graph_pointer, x_data, y_data );
	}

	this->rescaleAxes();
	this->yAxis->setRangeLower( 0 );
	double upper = this->yAxis->range().upper;
	this->yAxis->setRangeUpper( std::min( upper, 100. ) );
	this->replot();
}

//void Interactive_Graph::RegraphAll()
//{
//	for( int i = 0; i < this->graphCount(); i++ )
//	{
//		QCPGraph* active_graph = this->graph( i );
//	}
//}
//
void Interactive_Graph::saveCurrentGraph()
{
	QFileInfo file_name = QFileDialog::getSaveFileName( this,
														tr( "Save Graph" ), QString(), tr( "JPG File (*.jpg);; PNG File (*.png);; BMP File (*.bmp);; PDF File (*.pdf)" ) );

	int dpi = 1000;
	if( file_name.suffix().toLower() == "png" )
		this->savePng( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "jpg" )
	{
		double scale = (1.0 / 140) * dpi;

		this->saveJpg( file_name.absoluteFilePath(), 3 * dpi / scale, 2 * dpi / scale, scale, 75, dpi, QCP::ruDotsPerInch );
	}
	else if( file_name.suffix().toLower() == "bmp" )
		this->saveBmp( file_name.absoluteFilePath() );
	else if( file_name.suffix().toLower() == "pdf" )
	{
		this->savePdf( file_name.absoluteFilePath(), dpi * 3, dpi * 2 );
	}
}


void Interactive_Graph::UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data )
{
	//double previous_upper_limit = this->yAxis->range().upper;
	existing_graph->setData( x_data, y_data );
	//this->rescaleAxes();
	//this->yAxis->setRangeLower( 0 );
	//double upper = this->yAxis->range().upper;
	//this->yAxis->setRangeUpper( std::max( previous_upper_limit, std::min( upper, 1. ) ) );

	//this->yAxis->setRange( -10, 110 );
	this->replot();
}

const Single_Graph & Interactive_Graph::GetSelectedGraphData() const
{
	static Single_Graph nothing_selected;
	QList<QCPGraph*> selection = this->selectedGraphs();
	if( selection.size() == 0 )
		return nothing_selected;
	else
		return FindDataFromGraphPointer( selection[ 0 ] );
}

const Single_Graph & Interactive_Graph::FindDataFromGraphPointer( QCPGraph* graph_pointer ) const
{
	static Single_Graph nothing_selected;
	auto result = std::find_if(
		this->remembered_graphs.begin(),
		this->remembered_graphs.end(),
		[graph_pointer]( const auto& mo ) { return mo.second.graph_pointer == graph_pointer; } );

	//RETURN VARIABLE IF FOUND
	if( result != this->remembered_graphs.end() )
		return result->second;
	else
		return nothing_selected;
}

void Interactive_Graph::Set_As_Background( XY_Data xy )
{
	this->y_display_method = [xy]( double x, double y )
	{
		auto[ x_data, y_data ] = xy;
		int i = std::distance( x_data.begin(), std::lower_bound( x_data.begin(), x_data.end(), x ) );
		i = std::min( y_data.size() - 1, i );

		return (100 * y / y_data[ i ]);
	};
	this->RegraphAll();
}

void Interactive_Graph::Clear_Background()
{
	this->y_display_method = []( double x, double y ) { return y; };
	this->RegraphAll();
}

std::tuple< std::vector<double>, std::vector<double> > Load_XY_CSV_Data( const std::string & data_to_parse, const char* delimiter )
{
	//if constexpr (false)
	{
		using namespace std;
		using namespace boost;

		std::vector< std::string > split_by_line;
		boost::split( split_by_line, data_to_parse, boost::is_any_of( "\r\n" ), boost::algorithm::token_compress_on ); // this works for \r or \n file endings
		std::vector< std::tuple<double, double> > unsorted;
		unsorted.reserve( split_by_line.size() );
		for( const auto &[ line_number, one_line ] : split_by_line % fn::transform( fn::get::enumerated{} ) )
		{
			if( one_line.size() == 0 )
				continue; // Ignore blank lines

			std::vector< std::string > split_by_commas;
			split( split_by_commas, one_line, is_any_of( delimiter ) );
			if( split_by_commas.size() != 2 )
			{
				std::cerr << "Invalid formatting in data at line " + std::to_string( line_number + 1 ); // Line numbers usually start at 1 not zero
				continue;
			}

			unsorted.emplace_back( stod( split_by_commas[ 0 ] ), stod( split_by_commas[ 1 ] ) );
		}

		std::vector<double> x_data;
		std::vector<double> y_data;
		x_data.reserve( split_by_line.size() );
		y_data.reserve( split_by_line.size() );
		for( const auto &[ x, y ] : unsorted % fn::sort_by( []( const auto & data ) { return std::get<0>( data ); } ) )
		{
			x_data.push_back( x );
			y_data.push_back( y );
		}

		////auto test = meta::transpose(output);
		//const auto how_to_sort = make_sort_permutation( std::get<0>( output ) );
		//apply_permutation_in_place( std::get<0>( output ), how_to_sort );
		//apply_permutation_in_place( std::get<1>( output ), how_to_sort );

		return { x_data, y_data };
	}
	//{
	//	std::cin;
	//	using namespace ::ranges;

	//	std::ifstream data_file(file_name);
	//	//auto file_stream = istream_view<char>(data_file);
	//	//auto test = views::split([](char c) { return c == '\n' || c == '\r'; });
	//	//auto result = istream_view<char>(data_file) | views::split( [](auto const& c) { return c == '\n' || c == '\r'; } );
	//	std::vector< std::vector<double> > values_by_row_first = views::all(getlines(data_file)) | views::transform([](const auto line)
	//		{
	//			auto split_by_comma = line | views::split(',');
	//			auto change_to_doubles = views::all(split_by_comma)
	//				| views::transform([](auto s)
	//					{ return std::stod(s | to<std::string>); });
	//			return change_to_doubles | to<std::vector<double>>;
	//		}) | to<std::vector< std::vector<double> >>;

	//	auto values_by_column_first = meta::transpose(values_by_row_first);
	//}

	//std::ifstream in(file_name);
	//if (!in.is_open())
	//	return {};
	//
	//std::vector<char> test4 = fn::from( std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>{ /* end */ } ) % fn::to( std::vector<char>{} );

	//auto how_to_split_lines = []( const char ch )
	//{
	//	return ch != '\n' && ch != '\r';
	//};
	//auto split_by_line = fn::from( std::istreambuf_iterator<char>( in ), std::istreambuf_iterator<char>{ /* end */ } )
	//	% fn::group_adjacent_by( how_to_split_lines )
	//	% fn::foldl_d( [&]( std::vector<double> out, const std::string& w )
	//		{
	//			if( out.size() >= 2 )
	//				return std::move( out );
	//		} );
	//	//% fn::where( []( const auto ch ) { return ch != "\n" && ch != "\r"; } );
	//auto test2 = split_by_line
	//	% fn::transform([](auto one_line) -> std::array<double, 2>
	//		{
	//			//auto split_by_commas = std::move( one_line )
	//			//	% fn::group_adjacent_by( []( const char ch ) { return ch != ','; } )
	//			//	% fn::where( []( const auto ch ) { return ch != ","; } )
	//			//	% fn::transform( []( auto one_entry ) -> double
	//			//		{
	//			//			return std::stod( one_entry );
	//			//		} );
	//			//if( test.size() < 2 )
	//				return std::array<double, 2>{};
	//			//else
	//			//	return { one_line[ 0 ], one_line[ 1 ] };
	//		} );
	//auto values_by_row_first = test2 % fn::to( std::vector< std::string >{} );
	//auto values_by_row_first = test2 % fn::to_vector() % fn::to( std::vector< std::array<double, 2> >{} );
	//% fn::to_vector();
	//	% fn::for_each([this](const auto& line_of_elements)
	//		{
	//			if (line_of_elements.size() >= 2)
	//				Add_New_Material(QString::fromStdString(line_of_elements[0]),
	//					std::stod(line_of_elements[1]) * 1E6,
	//					std::stod(line_of_elements[2]));
	//		});

}
