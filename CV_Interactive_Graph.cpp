#include "CV_Interactive_Graph.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"

#include "fn.hpp"
namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));

//static QString Unit_Names[ 3 ] = { "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
//							"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
//							"Photon Energy (eV)" };

namespace CV
{

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent )
{
	this->y_axis_menu_functions.emplace_back( [this]( Graph_Base* graph, QMenu* menu, QPoint pos )
	{
		if( graph->yAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
		{
			//CURRENT_A = 0,
			//	CURRENT_A_PER_AREA_CM = 1,
			//	LOG_CURRENT_A = 2,
			//	LOG_CURRENT_A_PER_AREA_CM = 3,
			//	ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM = 4

			//menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
			//for( size_t index = 0; index < sizeof( Axes::Y_Unit_Names ) / sizeof( Axes::Y_Unit_Names[0] ); index++ )
			//{
			//	QString axis_name = Axes::Y_Unit_Names[ index ];
			//	menu->addAction( "Change to " + axis_name, [graph, index, axis_name, this]
			//	{
			//		graph->axes.y_units = Y_Units( index );
			//		graph->yAxis->setLabel( axis_name );
			//		graph->RegraphAll();
			//	} );
			//}
		}
	} );
}


Axes::Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
{
}

Prepared_Data Axes::Prepare_XY_Data( const Single_Graph & graph )
{
	arma::vec x_data = arma::conv_to<arma::vec>::from( graph.x_data.toStdVector() );
	arma::vec y_data = arma::conv_to<arma::vec>::from( graph.y_data.toStdVector() );
	return { toQVec( x_data ), toQVec( y_data ) };
}

void Axes::Graph_XY_Data( QString measurement_name, const Single_Graph & graph )
{
	auto[ x_data, y_data ] = this->Prepare_XY_Data( graph );
	graph.graph_pointer->setData( x_data, y_data );
}

const QString Axes::X_Unit_Names[ 1 ] = { "Voltage (V)" };
const QString Axes::Y_Unit_Names[ 5 ] = { "Capacitance (F)",
								QString::fromWCharArray( L"Current (A/cm\u00B2)" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|))" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ),
								QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ) };

}