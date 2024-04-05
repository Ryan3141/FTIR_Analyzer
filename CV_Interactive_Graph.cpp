#include "CV_Interactive_Graph.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"
#include <filesystem>
namespace fs = std::filesystem;
#include <regex>

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
	this->y_axis_menu_functions.emplace_back( [this]( QMenu* menu, QPoint pos )
	{
		if( this->yAxis->selectTest( pos, false ) >= 0 ) // general context menu on graphs requested
		{
			// CURRENT_A = 0,
			// 	CURRENT_A_PER_AREA_CM = 1,
			// 	LOG_CURRENT_A = 2,
			// 	LOG_CURRENT_A_PER_AREA_CM = 3,
			// 	ONE_SIDE_LOG_CURRENT_A_PER_AREA_CM = 4

			// menu->addAction( "Add random graph", this, SLOT( addRandomGraph() ) );
			for( size_t index = 0; index < sizeof( Axes::Y_Unit_Names ) / sizeof( Axes::Y_Unit_Names[0] ); index++ )
			{
				QString axis_name = Axes::Y_Unit_Names[ index ];
				menu->addAction( "Change to " + axis_name, [index, axis_name, this]
				{
					this->axes.y_units = Y_Units( index );
					this->yAxis->setLabel( axis_name );
					this->RegraphAll();
				} );
			}
		}
	} );

	this->general_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		menu->addAction( "Paste From Clipboard", [this]
		{
			const QClipboard *clipboard = QApplication::clipboard();
			const QMimeData *mimeData = clipboard->mimeData();

			if( mimeData->hasText() )
			{
				std::string clipboard_text = mimeData->text().toStdString();
				// Check if there is a label in the first line
				this->Add_Raw_Text_Data( clipboard_text );
			}
			else
			{
				//setText( tr( "Cannot display data" ) );
				return;
			}
		} );
	} );
}


Axes::Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
{
}

Prepared_Data Axes::Prepare_XY_Data( const Single_Graph & graph )
{
	arma::vec x_data = fromQVec( graph.x_data );
	arma::vec y_data = fromQVec( graph.y_data );
	switch( this->y_units )
	{
		case Y_Units::CAPACITANCE_F:
		break;
		case Y_Units::CAPACITANCE_F_PER_AREA_CM:
		{
			auto side_um = graph.meta.find( "device_side_length_in_um" );
			double side_cm = (side_um == graph.meta.end()) ? 1.0 : side_um->second.toDouble() * 1E-4;
			y_data /= side_cm * side_cm;
		}
		break;
	}
	return { toQVec( x_data ), toQVec( y_data ) };
}

void Axes::Graph_XY_Data( QString measurement_name, const Single_Graph & graph )
{
	auto[ x_data, y_data ] = this->Prepare_XY_Data( graph );
	graph.graph_pointer->setData( x_data, y_data );
}

const QString Axes::X_Unit_Names[ 1 ] = { "Voltage (V)" };
const QString Axes::Y_Unit_Names[ 2 ] = { "Capacitance (F)",
								QString::fromWCharArray( L"Capacitance per area (F/cm\u00B2)" ) };//,
								// QString::fromWCharArray( L"Current (log\u2081\u2080(|A|))" ),
								// QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ),
								// QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ) };

void Interactive_Graph::dropEvent(QDropEvent *event)
{
	if (event->source() == this && event->possibleActions() & Qt::MoveAction)
		return;
	if( !(event->proposedAction() == Qt::MoveAction || event->proposedAction() == Qt::CopyAction) )
		return;

	event->acceptProposedAction();

	const std::string drag_data = event->mimeData()->text().toStdString();
	std::vector< std::string > split_by_line;
	boost::split( split_by_line, drag_data, boost::is_any_of( "\n\r" ), boost::algorithm::token_compress_on );

	for( std::string file_path : split_by_line )
	{
		// file_path = replace( "file://", "" );
		// std::cout << "Loading file: " << fs::path{ file_path } << std::endl;
		// std::cout << "Loading stem: " << fs::path{ file_path }.stem() << std::endl;
		// std::cout << "lexically_normal: " << fs::path{ file_path }.lexically_normal() << std::endl;
		// std::cout << "root_name: " << fs::path{ file_path }.root_name() << std::endl;
#ifdef _WIN32
		if( file_path.size() < 8 )
			continue;
		fs::path cleaned_path{ file_path.substr( 8 ) };
#else
		if( file_path.size() < 7 )
			continue;
		fs::path cleaned_path{ file_path.substr( 7 ) };
#endif
		std::ifstream file{ cleaned_path };
		if( !file.is_open() )
		{
			std::cerr << "Could not open file: " << cleaned_path << std::endl;
			continue;
		}
		std::string file_contents(std::istreambuf_iterator<char>{file}, {});
		std::string just_filename = cleaned_path.stem().string();
		// Find in filename a number before "um" but only grab the number
		std::smatch match;
		std::regex_search( just_filename, match, std::regex( ".*?([0-9]+)um" ) );
		Labeled_Metadata metadata = { {"device_side_length_in_um", 10000.0} };
		if( !match.empty() )
		{
			double side_length = std::stod( match[1] );
			metadata[ "device_side_length_in_um" ] = side_length;
		}

		this->Add_Raw_Text_Data( file_contents, QString::fromStdString( cleaned_path.stem().string() ), metadata );
	}
}

void Interactive_Graph::Add_Raw_Text_Data( const std::string & text_data, std::optional<QString> title, Labeled_Metadata metadata )
{
	std::vector< std::string > split_by_line;
	boost::split( split_by_line, text_data, boost::is_any_of( "\n\r" ), boost::algorithm::token_compress_on );
	std::vector< std::string > split_by_delimeter;
	boost::split( split_by_delimeter, split_by_line[0], boost::is_any_of( ",\t" ) );
	QString label = split_by_delimeter.size() > 1 ? title.value_or("Data") : QString::fromStdString( split_by_delimeter[0] );
	int skip_header = split_by_delimeter.size() > 1 ? 0 : 1;
	for( ;; skip_header++ )
	{
		if( split_by_line[ skip_header ].find_first_not_of( "0123456789.+-Ee,\t" ) == std::string::npos )
			break;
	}

	auto [x_data, y_data] = Load_XY_CSV_Data( text_data, ",\t", skip_header, true );
	if( x_data.size() == 0 || y_data.size() == 0 )
		return;
	static int index = 0;

	QString measurement_name = "Clipboard Data " + QString::number( index++ );
	Single_Graph & new_graph =
		this->Graph<X_Units::VOLTAGE_V, Y_Units::CAPACITANCE_F>( toQVec( x_data ),
					toQVec( y_data ), measurement_name, label, metadata );
	this->replot();
}

}