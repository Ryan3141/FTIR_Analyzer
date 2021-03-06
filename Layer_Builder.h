#pragma once

#include <string>
#include <optional>

#include <QtCore/QObject>
#include <QListWidget>
#include <QFileInfo>

#include "Material_Layer_Widget.h"

#include "Thin_Film_Interference.h"

class Layer_Builder : public QListWidget
{
	Q_OBJECT

public:
	Layer_Builder( QWidget *parent = Q_NULLPTR );
	void Set_Material_List( std::map<std::string, Material> new_name_to_material );
	std::vector<Material_Layer> Build_Material_List( std::optional< double > temperature ) const;
	void Make_From_Material_List( const std::vector<Material_Layer> & materials );

	void Load_From_File( QFileInfo file );
	void Save_To_File( QFileInfo file ) const;

	//class Header : public QWidget
	//{
	//public:
	//	/*!
	//	 * \brief Constructor used to defined the parent/child relation
	//	 *        between the Header and the QListView.
	//	 * \param parent Parent of the widget.
	//	 */
	//	Header( Layer_Builder* parent );

	//	/*!
	//	 * \brief Overridden method which allows to get the recommended size
	//	 *        for the Header object.
	//	 * \return The recommended size for the Header widget.
	//	 */
	//	QSize sizeHint() const;

	//protected:
	//	/*!
	//	 * \brief Overridden paint event which will allow us to design the
	//	 *        Header widget area and draw some text.
	//	 * \param event Paint event.
	//	 */
	//	void paintEvent( QPaintEvent* event );

	//private:
	//	Layer_Builder* menu;    /*!< The parent of the Header. */
	//};

private:
	void Add_New_Material( Optional_Material_Parameters parameters );
	void Add_New_Material( std::string material_name,
						   double thickness,
						   std::optional< double > composition = {},
						   std::optional< double > tauts_gap = {},
						   std::optional< double > urbach_energy = {} );

	void dropEvent( QDropEvent* event );

	QStringList material_names;
	std::map<std::string, Material> name_to_material;


signals:
	void Materials_List_Changed();
};

//#include <QWidget>
//#include <QTableWidget>
//#include <QDropEvent>
//#include <QDragMoveEvent>
//#include <QDropEvent>
//#include <QMimeData>
//#include <QDebug>
//#include <QKeyEvent>
//
//class DTableWidget : public QTableWidget
//{
//	Q_OBJECT
//
//public:
//	DTableWidget( QWidget *parent = 0 );
//
//public slots:
//
//signals:
//	void keyboard( QKeyEvent *event );
//	void dropped( const QMimeData* mimeData = 0 );
//	void moved( int old_row, int new_row );
//
//protected:
//	void dragEnterEvent( QDragEnterEvent *event );
//	void dragMoveEvent( QDragMoveEvent *event );
//	void dragLeaveEvent( QDragLeaveEvent *event );
//	void dropEvent( QDropEvent *event );
//	void startDrag( Qt::DropActions supportedActions ) override;
//	QList<QTableWidgetItem*> takeRow( int row );
//	void setRow( int row, const QList<QTableWidgetItem*>& rowItems );
//
//private:
//};

