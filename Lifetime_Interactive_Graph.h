#pragma once

#include <tuple>
#include <QVector>
#include <functional>
#include <optional>

#include "Interactive_Graph.h"


namespace Lifetime
{

enum class X_Units
{
	TIME_US = 0,
	TEMPERATURE_K,
	THOUSAND_OVER_TEMPERATURE_K,
	FIT_TIME_US,
	FIT_TIME_US2,
	DONT_CHANGE
};

enum class Y_Units
{
	VOLTAGE_V = 0,
	TIME_US,
	TIME_US2,
	FIT_VOLTAGE_V,
	LOG_FIT_VOLTAGE_V,
	DONT_CHANGE
};

struct Fit_Results
{
	double amplitude;
	double x_offset;
	double lifetime;
};

struct Single_Graph : public Default_Single_Graph<X_Units, Y_Units>
{
	Fit_Results early_fit;
	Fit_Results late_fit;
	QCPGraph* early_fit_graph = nullptr;
	QCPGraph* late_fit_graph = nullptr;

	double lower_x_fit = 0.1E-6;
	double upper_x_fit = 4.0E-6;
	double upper_x_fit2 = 4.0E-6;
	double x_offset = 0.0;
	double y_offset = 0.0;
	double lowpass_MHz = -1.0;
	std::vector<QCPGraph*> Get_Graphs() const
	{
		return { graph_pointer, early_fit_graph };
	}
	void SetColor( const QColor& color ) const
	{
		QPen pen = graph_pointer->pen();
		pen.setColor( color );
		graph_pointer->setPen( pen );
		if( early_fit_graph != nullptr )
		{
			QPen pen2 = early_fit_graph->pen();
			pen2.setColor( color );
			early_fit_graph->setPen( pen2 );
		}
	}
};

const int units_count = 5;
struct Axes
{
	using XY_Data = std::tuple< QVector<double>, QVector<double> >;

	const static X_Units default_x_units = X_Units::TIME_US;
	const static Y_Units default_y_units = Y_Units::VOLTAGE_V;
	X_Units x_units = default_x_units;
	Y_Units y_units = default_y_units;
	std::function<void()> graph_function;

	Axes( std::function<void()> regraph_function ) : graph_function( regraph_function )
	{
	}

	void Set_X_Units( X_Units units );
	void Set_Y_Units( Y_Units units );

	Prepared_Data Prepare_Any_Data( arma::vec x, arma::vec y, X_Units x_units, double x_offset = 0, double y_offset = 0,
									std::optional<double> lowpass_Hz = std::nullopt,
									std::optional<double> sampling_frequency = std::nullopt ) const;
	Prepared_Data Prepare_Fit_Data( const arma::vec& x, const arma::vec& y ) const;
	Prepared_Data Prepare_XY_Data( const Single_Graph & graph_data ) const;
	void Graph_XY_Data( QString measurement_name, const Single_Graph & graph );

	QVector<double> background_x_data;
	QVector<double> background_y_data;

	const static QString X_Unit_Names[ units_count ];
	const static QString Y_Unit_Names[ units_count ];
	const static QString Change_To_X_Unit_Names[ units_count ];
	const static QString Change_To_Y_Unit_Names[ units_count ];
};

using Graph_Base = ::Interactive_Graph<Single_Graph, Axes>;

class Interactive_Graph :
	public Graph_Base
{
private:
	QSharedPointer<QCPAxisTicker> linearTicker;
	QSharedPointer<QCPAxisTickerLog> logTicker;
	QCPRange remembered_ranges_x[ units_count ] = { { 0, 100 }, { 60, 320 }, { 1.0/320, 1.0/60 }, { -1, 21 }, {   -1, 21 } };
	QCPRange remembered_ranges_y[ units_count ] = { { 0,   5 }, {  0,  10 }, {       0,     10 }, {  0,  1 }, { 1E-3,  1 } };
	enum Fit_Technique
	{
		PIECEWISE = 0,
		CPPAD = 1,
		CERES = 2,
	};
public:
	Interactive_Graph( QWidget* parent = nullptr );
	void Redo_Fits( std::vector<std::tuple<QString, Single_Graph&>> graphs_for_fit );
	void Hide_Fit_Graphs( Single_Graph & single_graph, bool should_hide );

	void Change_Axes( int index );
	Fit_Technique fit_technique = Fit_Technique::CERES;
};

template< typename FloatType >
constexpr FloatType Convert_Units( X_Units original, X_Units converted, FloatType input )
{
	return input;
}

}