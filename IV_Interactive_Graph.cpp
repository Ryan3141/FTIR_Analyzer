#include "IV_Interactive_Graph.h"

#include <algorithm>
#include <armadillo>

#include "Units.h"
#include "Ceres_Curve_Fitting.h"
#include <filesystem>
namespace fs = std::filesystem;
#include <regex>

#include "boost/algorithm/string.hpp"
#include "rangeless_helper.hpp"

//static QString Unit_Names[ 3 ] = { "Wave Number (cm" + QString( QChar( 0x207B ) ) + QString( QChar( 0x00B9 ) ) + ")",
//							"Wavelength (" + QString( QChar( 0x03BC ) ) + "m)",
//							"Photon Energy (eV)" };

namespace IV
{

class QCPAxisTickerTemperature : public QCPAxisTicker
{
public:
	int mSubTickCount = 8;

	QCPAxisTickerTemperature()
	{
	}


	void setSubTickCount(int subTicks)
	{
		if (subTicks >= 0)
			mSubTickCount = subTicks;
		else
			qDebug() << Q_FUNC_INFO << "sub tick count can't be negative:" << subTicks;
	}

  int subTickCount() const { return mSubTickCount; }
  
protected:
  // reimplemented virtual methods:
	virtual double getTickStep(const QCPRange &range) override
	{
		// Logarithmic axis ticker has unequal tick spacing, so doesn't need this method
		Q_UNUSED(range)
		return 1.0;
	}

	virtual int getSubTickCount(double tickStep) override
	{
		Q_UNUSED(tickStep)
		return mSubTickCount;
	}

	virtual QVector<double> createTickVector(double tickStep, const QCPRange &range) override
	{
		Q_UNUSED(tickStep)
		QVector<double> result = { 80, 100, 150, 200, 250, 300 };
		// if (range.lower > 0 && range.upper > 0) // positive range
		// {
		// 	double exactPowerStep =  qLn(range.upper/range.lower)*mLogBaseLnInv/(double)(mTickCount+1e-10);
		// 	double newLogBase = qPow(mLogBase, qMax((int)cleanMantissa(exactPowerStep), 1));
		// 	double currentTick = qPow(newLogBase, qFloor(qLn(range.lower)/qLn(newLogBase)));
		// 	result.append(currentTick);
		// 	while (currentTick < range.upper && currentTick > 0) // currentMag might be zero for ranges ~1e-300, just cancel in that case
		// 	{
		// 		currentTick *= newLogBase;
		// 		result.append(currentTick);
		// 	}
		// } else if (range.lower < 0 && range.upper < 0) // negative range
		// {
		// 	double exactPowerStep =  qLn(range.lower/range.upper)*mLogBaseLnInv/(double)(mTickCount+1e-10);
		// 	double newLogBase = qPow(mLogBase, qMax((int)cleanMantissa(exactPowerStep), 1));
		// 	double currentTick = -qPow(newLogBase, qCeil(qLn(-range.lower)/qLn(newLogBase)));
		// 	result.append(currentTick);
		// 	while (currentTick < range.upper && currentTick < 0) // currentMag might be zero for ranges ~1e-300, just cancel in that case
		// 	{
		// 		currentTick /= newLogBase;
		// 		result.append(currentTick);
		// 	}
		// } else // invalid range for logarithmic scale, because lower and upper have different sign
		// {
		// 	qDebug() << Q_FUNC_INFO << "Invalid range for temperature plot: " << range.lower << ".." << range.upper;
		// }

		return result;
	}
};

void Interactive_Graph::Add_Color_Bar()
{
	colorScale = new QCPColorScale( this );
	colorScale->setLabel("Temperature (K)");
	colorScale->setGradient( QCPColorGradient::gpSpectrum );
	colorScale->axis()->setRange( 78, 302 );
	colorScale->axis()->setRangeReversed( true );
	// colorScale->axis()->setNumberFormat( "ebd" );
	// colorScale->axis()->setNumberPrecision( 0 );
	// colorScale->axis()->setTicker(QSharedPointer<QCPAxisTickerLog>(new QCPAxisTickerLog));
	colorScale->axis()->setTicker(QSharedPointer<QCPAxisTickerTemperature>(new QCPAxisTickerTemperature));

	colorScale->setRangeZoom( false );
	colorScale->setRangeDrag( false );
	// colorScale->setVisible( false );
	this->plotLayout()->addElement(0, 1, this->colorScale);
}

void Interactive_Graph::Remove_Color_Bar()
{
	this->plotLayout()->remove( this->colorScale );
	this->plotLayout()->simplify();
}

Interactive_Graph::Interactive_Graph( QWidget* parent ) :
	Graph_Base( parent ),
	linearTicker( new QCPAxisTicker ),
	logTicker( new QCPAxisTickerLog )
{
	this->yAxis->setTicker( linearTicker );
	this->yAxis2->setTicker( linearTicker );
	this->y_axis_menu_functions.emplace_back( [ this ]( QMenu* menu, QPoint pos ) mutable
	{
		for( size_t index = 0; index < sizeof( Axes::Y_Unit_Names ) / sizeof( Axes::Y_Unit_Names[ 0 ] ); index++ )
		{
			menu->addAction( Axes::Change_To_Y_Unit_Names[ index ], [ index, this ]
			{
				Change_Y_Axis( index );
			} );
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

	auto [x_data, y_data] = Load_XY_CSV_Data( text_data, ",\t", skip_header );
	if( x_data.size() == 0 || y_data.size() == 0 )
		return;
	static int index = 0;

	QString measurement_name = "Clipboard Data " + QString::number( index++ );
	const Single_Graph & selected = this->GetSelectedGraphData();
	Single_Graph & new_graph =
		this->Graph<X_Units::VOLTAGE_V, Y_Units::CURRENT_A>( toQVec( x_data ),
					toQVec( y_data ), measurement_name, label, metadata );
	if( selected.graph_pointer != nullptr )
	{
		double slope = Ceres_Fit_Resistor( selected, new_graph );
		new_graph.resistance = slope;
		this->axes.Graph_XY_Data( measurement_name, new_graph ); // Replot with updated resistance
	}
	this->replot();
}

void Interactive_Graph::Change_X_Axis( int index )
{
	X_Units x_units = X_Units( index );
}

void Interactive_Graph::Change_Y_Axis( int index )
{
	Y_Units y_units = Y_Units( index );
	this->axes.y_units = y_units;
	if( Y_Units::LOG_CURRENT_A == this->axes.y_units ||
		Y_Units::LOG_CURRENT_A_PER_AREA_CM == this->axes.y_units )
	{
		for( auto axis : { this->yAxis, this->yAxis2 } )
		{
			axis->setScaleType( QCPAxis::stLogarithmic );
			axis->setTicker( logTicker );
			axis->setNumberFormat( "ebd" );
			axis->setNumberPrecision( 0 );
			if( axis->range().lower < 0 )
				axis->setRangeLower( 1E-15 );
			if( axis->range().upper < 0 )
				axis->setRangeUpper( 1E-3 + axis->range().lower );
		}
	}
	else
	{
		for( auto axis : { this->yAxis, this->yAxis2 } )
		{
			axis->setScaleType( QCPAxis::stLinear );
			axis->setTicker( linearTicker );
			axis->setNumberFormat( "gbd" );
			axis->setNumberPrecision( 6 );
		}
	}
	const QString & axis_name = Axes::Y_Unit_Names[ static_cast<int>( y_units ) ];
	this->yAxis->setLabel( axis_name );
	this->RegraphAll();
}

const QString Axes::X_Unit_Names[ 1 ] = { "Voltage (V)" };
const QString Axes::Change_To_Y_Unit_Names[ 7 ] = { "Change to current",
								"Change to current per area",
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current )" ),
								QString::fromWCharArray( L"Change to log\u2081\u2080( Current per area )" ),
								QString::fromWCharArray( L"Change to one-sided log\u2081\u2080( Current per area )" ),
								QString::fromWCharArray( L"Change to resistance" ),
								QString::fromWCharArray( L"Change to resistance per area" ) };
const QString Axes::Y_Unit_Names[ 7 ] = { "Current (A)",
								QString::fromWCharArray( L"Current (A/cm\u00B2)" ),
								QString::fromWCharArray( L"Current |A|" ),
								QString::fromWCharArray( L"Current |A/cm\u00B2|" ),
								QString::fromWCharArray( L"Current |A/cm\u00B2|" ),
								QString::fromWCharArray( L"Resistance (\u03A9)" ),
								QString::fromWCharArray( L"Resistance (\u03A9/cm\u00B2)" ) };
//QString::fromWCharArray( L"Current (log\u2081\u2080(|A|/cm\u00B2))" ),

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

}