﻿#pragma once
#include "Interactive_Graph.h"

#include <array>
#include <algorithm>
#include <armadillo>
#include <functional>


#include "rangeless_helper.hpp"


template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::Interactive_Graph( QWidget *parent ) :
	Interactive_Graph_QObject_Adapter( parent ),
	axes( [ this ]() { this->RegraphAll(); } )
{
	Initialize_Graph();
}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::Initialize_Graph()
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

	this->xAxis->setLabel( Axes_Scales::X_Unit_Names[ int( Axes_Scales::default_x_units ) ] );
	this->yAxis->setLabel( Axes_Scales::Y_Unit_Names[ int( Axes_Scales::default_y_units ) ] );
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
	connect( this, &QWidget::customContextMenuRequested, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::graphContextMenuRequest );
	connect( this, &QCustomPlot::selectionChangedByUser, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::selectionChanged );
	// connect slots that takes care that when an axis is selected, only that direction can be dragged and zoomed:
	connect( this, &QCustomPlot::mousePress, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::mousePress );
	connect( this, &QCustomPlot::mouseWheel, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::mouseWheel );

	connect( this, &QCustomPlot::mouseDoubleClick, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::refitGraphs );

	// make bottom and left axes transfer their ranges to top and right axes:
	connect( this->xAxis, SIGNAL( rangeChanged( QCPRange ) ), this->xAxis2, SLOT( setRange( QCPRange ) ) );
	connect( this->yAxis, SIGNAL( rangeChanged( QCPRange ) ), this->yAxis2, SLOT( setRange( QCPRange ) ) );

	// connect some interaction slots:
	connect( this, &QCustomPlot::axisDoubleClick, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::axisLabelDoubleClick );
	connect( this, &QCustomPlot::legendDoubleClick, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::legendDoubleClick );
	//connect( title, &QCPTextElement::doubleClicked, this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::titleDoubleClick );

	//// connect slot that shows a message in the status bar when a graph is clicked:
	//connect( this, SIGNAL( plottableClick( QCPAbstractPlottable*, int, QMouseEvent* ) ), this, SLOT( graphClicked( QCPAbstractPlottable*, int ) ) );

	// setup policy and connect slot for context menu popup:
	this->setContextMenuPolicy( Qt::CustomContextMenu );
}

//template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
//void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::mouseDoubleClickEvent( QMouseEvent* event )
//{
//	QCustomPlot::mouseDoubleClickEvent( event );
//
//	//if( !event->isAccepted() )
//	//	this->refitGraphs( event );
//}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::titleDoubleClick( QMouseEvent* event )
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::axisLabelDoubleClick( QCPAxis *axis, QCPAxis::SelectablePart part, QMouseEvent * event )
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::legendDoubleClick( QCPLegend *legend, QCPAbstractLegendItem *item, QMouseEvent * event )
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::selectionChanged()
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::mousePress()
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::mouseWheel()
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::refitGraphs( QMouseEvent* event )
{
	event->accept();
	this->rescaleAxes( true );
	//this->yAxis->setRangeLower( 0 );
	//this->yAxis->setRange( -10, 110 );
	this->replot();
}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::removeSelectedGraph()
{
	if( this->selectedGraphs().size() == 0 )
		return;

	QCPGraph* selected_graph = this->selectedGraphs().first();
	for( std::map< QString, Single_Graph< X_Unit_Type, Y_Unit_Type > >::iterator element = remembered_graphs.begin();
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::removeAllGraphs()
{
	this->clearGraphs();
	this->remembered_graphs.clear();
	this->replot();
}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::graphContextMenuRequest( QPoint pos )
{
	QMenu *menu = new QMenu( this );
	menu->setAttribute( Qt::WA_DeleteOnClose );

	bool selected_legend = this->legend->visible() && this->legend->selectTest( pos, false ) >= 0;
	bool selected_x_axis = this->xAxis->selectTest( pos, false ) >= 0;
	bool selected_y_axis = this->yAxis->selectTest( pos, false ) >= 0;

	if( selected_legend ) // context menu on legend requested
	{
		menu->addAction( "Move to top left", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignLeft) );
		menu->addAction( "Move to top center", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignHCenter) );
		menu->addAction( "Move to top right", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::moveLegend )->setData( (int)(Qt::AlignTop | Qt::AlignRight) );
		menu->addAction( "Move to bottom right", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignRight) );
		menu->addAction( "Move to bottom left", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::moveLegend )->setData( (int)(Qt::AlignBottom | Qt::AlignLeft) );
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
			menu->addAction( "Remove selected graph", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::removeSelectedGraph );
		if( this->graphCount() > 0 )
			menu->addAction( "Remove all graphs", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::removeAllGraphs );
		menu->addAction( "Save current graph", this, &Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::saveCurrentGraph );

		for( const auto & menu_function : this->general_menu_functions )
			menu_function( this, menu, pos );
	}

	if( selected_x_axis )
		for( const auto & menu_function : this->x_axis_menu_functions )
			menu_function( this, menu, pos );

	if( selected_y_axis )
		for( const auto & menu_function : this->y_axis_menu_functions )
			menu_function( this, menu, pos );

	menu->popup( this->mapToGlobal( pos ) );
}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::moveLegend()
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::graphClicked( QCPAbstractPlottable *plottable, int dataIndex )
{
	//// since we know we only have QCPGraphs in the plot, we can immediately access interface1D()
	//// usually it's better to first check whether interface1D() returns non-zero, and only then use it.
	//double dataValue = plottable->interface1D()->dataMainValue( dataIndex );
	//QString message = QString( "Clicked on graph '%1' at data point #%2 with value %3." ).arg( plottable->name() ).arg( dataIndex ).arg( dataValue );
	//ui.statusBar->showMessage( message, 2500 );
}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::Hide_Graph( QString graph_name )
{
	auto existing_graph = remembered_graphs.find( graph_name );
	if( existing_graph == remembered_graphs.end() )
		return;

	Single_Graph< X_Unit_Type, Y_Unit_Type > & current_info = existing_graph->second;
	if( !current_info.graph_pointer->visible() ) // Already invisible, nothing to do
		return;
	current_info.graph_pointer->setVisible( false );
	current_info.graph_pointer->removeFromLegend();
	replot();
}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::RegraphAll()
{
	for( const auto &[ name, graph ] : remembered_graphs )
	{
		auto[ x_data, y_data ] = axes.Prepare_XY_Data( graph );

		this->UpdateGraph( graph.graph_pointer, x_data, y_data );
	}

	//this->rescaleAxes();
	//this->yAxis->setRangeLower( 0 );
	//double upper = this->yAxis->range().upper;
	//this->yAxis->setRangeUpper( std::min( upper, 100. ) );
	this->replot();
}

//void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::RegraphAll()
//{
//	for( int i = 0; i < this->graphCount(); i++ )
//	{
//		QCPGraph* active_graph = this->graph( i );
//	}
//}
//
template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::saveCurrentGraph()
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


template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
void Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::UpdateGraph( QCPGraph* existing_graph, QVector<double> x_data, QVector<double> y_data )
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

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
const Single_Graph< X_Unit_Type, Y_Unit_Type > & Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::GetSelectedGraphData() const
{
	static Single_Graph< X_Unit_Type, Y_Unit_Type > nothing_selected;
	QList<QCPGraph*> selection = this->selectedGraphs();
	if( selection.size() == 0 )
		return nothing_selected;
	else
		return FindDataFromGraphPointer( selection[ 0 ] );
}

template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
const Single_Graph< X_Unit_Type, Y_Unit_Type > & Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::FindDataFromGraphPointer( QCPGraph* graph_pointer ) const
{
	static Single_Graph< X_Unit_Type, Y_Unit_Type > nothing_selected;
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


template<typename X_Unit_Type, typename Y_Unit_Type, typename Axes_Scales>
template< X_Unit_Type X_Units, Y_Unit_Type Y_Units >
const Single_Graph< X_Unit_Type, Y_Unit_Type > & Interactive_Graph<X_Unit_Type, Y_Unit_Type, Axes_Scales>::Graph( QVector<double> x_data, QVector<double> y_data, QString measurement_name, QString graph_title, Labeled_Metadata meta )
{
	if( x_data.size() != y_data.size() )
	{
		std::cerr << "Incompatible sizes being graphed, size(x) = " << x_data.size() << " size(y) = " << y_data.size() << "\n";
		if( x_data.size() > y_data.size() )
			x_data.resize( y_data.size() );
		else
			y_data.resize( x_data.size() );
	}

	//for( auto & x : x_data )
	//	if( !std::isnormal( x ) )
	//		x = qQNaN();
	//for( auto & y : y_data )
	//	if( !std::isfinite( y ) || y > 1.0E4 )
	//		y = qQNaN();

	auto existing_graph = remembered_graphs.find( measurement_name );
	if( existing_graph != remembered_graphs.end() )
	{
		auto & current_info = existing_graph->second;
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
		remembered_graphs[ measurement_name ] = Single_Graph< X_Unit_Type, Y_Unit_Type >{ X_Units, Y_Units, std::move( x_data ), std::move( y_data ), current_graph, meta };
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
