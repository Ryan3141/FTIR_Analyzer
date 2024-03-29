#include "Layer_Builder.h"

#include <QComboBox>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>

#include "fn.hpp"
namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));

#include "Material_Layer_Widget.h"

Layer_Builder::Layer_Builder( QWidget *parent )
	: QListWidget(parent)
{
}

void Layer_Builder::Load_From_File( QFileInfo file )
{
	this->clear();
	std::ifstream in( file.absoluteFilePath().toStdString() );
	if( !in.is_open() )
		return;

	fn::from( std::istreambuf_iterator<char>( in ),
			  std::istreambuf_iterator<char>{ /* end */ } )
		% fn::group_adjacent_by( []( const char ch ) { return ch != '\n' && ch != '\r'; } )
		% fn::where( []( const auto ch ) { return ch != "\n" && ch != "\r"; } )
		% fn::transform( []( auto one_line )
			{
				return std::move( one_line ) % fn::group_adjacent_by( []( const char ch ) { return ch != ','; } )
					% fn::where( []( const auto ch ) { return ch != ","; } );
			} )
		% fn::for_each( [this]( const auto & line_of_elements )
			{
				if( line_of_elements.size() >= 3 )
					Add_New_Material( line_of_elements[ 0 ],
									  std::stod( line_of_elements[ 1 ] ),
									  std::stod( line_of_elements[ 2 ] ) );
			} );

	Add_New_Material( Optional_Material_Parameters{ "" } ); // Load blank entry for further filling in
	emit Materials_List_Changed();
}

void Layer_Builder::Save_To_File( QFileInfo file ) const
{
	std::ofstream out( file.absoluteFilePath().toStdString() );
	if( !out.is_open() )
		return;

	std::vector<Material_Layer> layers = Build_Material_List( std::nullopt );
	for( const Material_Layer & layer : layers )
	{
		auto material_name = std::find_if( std::begin( name_to_material ), std::end( name_to_material ), [layer]( const auto& mo ) { return mo.second == layer.material; } );
		if( material_name == name_to_material.end() )
			continue;
		out << material_name->first;
		out << "," << layer.parameters.thickness.value_or( 0.0 );
		out << "," << layer.parameters.composition.value_or( 0.0 );
		out << "\n";
	}
}

void Layer_Builder::dropEvent( QDropEvent* event )
{
	QListWidget::dropEvent( event );
	emit Materials_List_Changed();
}

void Layer_Builder::Add_New_Material( std::string material_name,
									  double thickness,
									  std::optional< double > composition,
									  std::optional< double > tauts_gap,
									  std::optional< double > urbach_energy )
{
	Optional_Material_Parameters parameters( material_name,
											 std::nullopt, // Temperature
											 thickness,
											 composition,
											 tauts_gap,
											 urbach_energy );
	Add_New_Material( parameters );
}

void Layer_Builder::Add_New_Material( Optional_Material_Parameters parameters )
{
	if( !material_names.contains( QString::fromStdString( parameters.material_name ) ) && parameters.material_name != "" )
		return;
	QListWidgetItem *iconItem = new QListWidgetItem( this );
	Material_Layer_Widget* one_material = new Material_Layer_Widget( this, material_names, parameters );
	connect( one_material, &Material_Layer_Widget::New_Material_Created, [ this ] { this->Add_New_Material( Optional_Material_Parameters{ "" } ); } ); // Add a new final blank entry to allow for a new one to be created
	connect( one_material, &Material_Layer_Widget::Material_Values_Changed, [ this ] { emit Materials_List_Changed(); } );
	connect( one_material, &Material_Layer_Widget::Material_Changed, [ this, one_material ]
	{
		auto [new_parameters, should_fit] = one_material->Get_Details();
		auto find_mat_in_defaults = defaults_per_material.find( name_to_material[ new_parameters.material_name ] );
		//if( find_mat_in_defaults == defaults_per_material.end() )
		//	return;
		auto [ mat, defaults ] = *find_mat_in_defaults;

		bool oldState = one_material->blockSignals( true ); // Prevent remove from triggering another changed signal while we edit things
		for( auto [ widget, one_default ] : fn::zip( one_material->double_widgets, defaults ) )
		{
			if( one_default.has_value() )
			{
				if( !widget->isEnabled() )
				{ // Only write default over if it was previously not used
					widget->setEnabled( true );
					widget->setValue( one_default.value() );
				}
			}
			else
			{
				widget->use_in_fit = false;
				widget->setEnabled( false );
			}
		}
		one_material->blockSignals( oldState );

		emit Materials_List_Changed();
	} );

	connect( one_material, &Material_Layer_Widget::Delete_Requested, [iconItem, this]
	{
		this->removeItemWidget( iconItem );
		delete iconItem;
		emit Materials_List_Changed();
	} );

	iconItem->setSizeHint( one_material->sizeHint() );
	this->addItem( iconItem );
	this->setItemWidget( iconItem, one_material );
}

std::vector<Material_Layer> Layer_Builder::Build_Material_List( std::optional< double > temperature_in_k ) const
{
	std::vector<Material_Layer> output;

	for( int i = 0; i < this->count(); i++ )
	{
		const Material_Layer_Widget* layer = static_cast<const Material_Layer_Widget*>( this->itemWidget( this->item( i ) ) );
		auto [p, what_to_fit] = layer->Get_Details();
		p.temperature = temperature_in_k;
		if( p.material_name == "" )
			continue;

		auto find_mat = name_to_material.find( p.material_name );
		if( find_mat == name_to_material.end() )
			continue;

		Material_Layer one_layer( find_mat->second, p );
		one_layer.what_to_fit = what_to_fit;
		output.push_back( std::move( one_layer ) );
	}
	return output;
}

void Layer_Builder::Make_From_Material_List( const std::vector<Material_Layer> & layers )
{
	for( int i = 0; const Material_Layer & layer : layers )
	{
		Material_Layer_Widget* layer_widget = static_cast<Material_Layer_Widget*>( this->itemWidget( this->item( i ) ) );

		layer_widget->Update_Values( layer.parameters );
		i++;
	}
}

void Layer_Builder::Set_Material_List( std::map<std::string, Material> new_name_to_material )
{
	for( const auto &[ name, material ] : new_name_to_material )
		material_names.push_back( QString::fromStdString( name ) );

	this->name_to_material = std::move( new_name_to_material );
	Add_New_Material( Optional_Material_Parameters{ "" } );
}

//#include <QtGui>
//
//DTableWidget::DTableWidget( QWidget *parent ) : QTableWidget( parent )
//{
//	//set widget default properties:
//	setFrameStyle( QFrame::Sunken | QFrame::StyledPanel );
//	setDropIndicatorShown( true );
//	setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
//	setEditTriggers( QAbstractItemView::NoEditTriggers );
//	setDragDropMode( QAbstractItemView::DropOnly );
//	setAlternatingRowColors( true );
//	setSelectionBehavior( QAbstractItemView::SelectRows );
//	setShowGrid( false );
//	setAcceptDrops( true );
//	setWordWrap( false );
//	setStyleSheet( "selection-background-color: yellow;"
//				   "selection-color: #002041;"
//				   "font-size: 75%;"
//	);
//	//setStyle( new NoFocusProxyStyle() );
//
//}
//
//void DTableWidget::dragEnterEvent( QDragEnterEvent *event )
//{
//	QObject* test = event->source();
//	event->acceptProposedAction();
//}
//
//void DTableWidget::dragMoveEvent( QDragMoveEvent *event )
//{
//	event->acceptProposedAction();
//}
//
//void DTableWidget::dropEvent( QDropEvent *event )
//{
//
//	event->acceptProposedAction();
//
//	if( event->mimeData()->urls().size() > 0 )
//		emit dropped( event->mimeData() );
//	else
//	{
//		QPoint old_coordinates = QPoint( -1, -1 );
//		int dropAction = event->dropAction();
//		if( currentItem() != NULL ) //Check if user is not accessing empty cell
//		{
//			old_coordinates = QPoint( currentItem()->row(), currentItem()->column() );
//		}
//		QTableWidget::dropEvent( event );
//		qDebug() << "Detected drop event...";
//		if( this->itemAt( event->pos().x(), event->pos().y() ) != NULL && old_coordinates != QPoint( -1, -1 ) )
//		{
//			qDebug() << "Drop Event Accepted.";
//			qDebug() << "Source: " << old_coordinates.x() << old_coordinates.y()
//				<< "Destinition: " << this->itemAt( event->pos().x(), event->pos().y() )->row()
//				<< this->itemAt( event->pos().x(), event->pos().y() )->column()
//				<< "Type: " << dropAction;
//
//			emit moved( old_coordinates.x(), itemAt( event->pos().x(), event->pos().y() )->row() );
//
//		}
//	}
//}
//
//void DTableWidget::dragLeaveEvent( QDragLeaveEvent *event )
//{
//	event->accept();
//}
//
//void DTableWidget::startDrag( Qt::DropActions supportedActions )
//{
//	QTableWidgetItem* item = currentItem();
//	QDrag* drag = new QDrag( this );
//	auto test = drag->exec( Qt::MoveAction );
//	if( test == Qt::MoveAction )
//	{
//		takeRow( row( item ) );
//		//delete takeRow( row( item ) );
//		//emit itemDroped();
//	}
//}
//
//// takes and returns the whole row
//QList<QTableWidgetItem*> DTableWidget::takeRow( int row )
//{
//	QList<QTableWidgetItem*> rowItems;
//	for( int col = 0; col < columnCount(); ++col )
//	{
//		rowItems << takeItem( row, col );
//	}
//	return rowItems;
//}
//
//// sets the whole row
//void DTableWidget::setRow( int row, const QList<QTableWidgetItem*>& rowItems )
//{
//	for( int col = 0; col < columnCount(); ++col )
//	{
//		setItem( row, col, rowItems.at( col ) );
//	}
//}