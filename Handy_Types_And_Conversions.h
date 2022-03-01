#pragma once

#include <armadillo>
#include <QVector>
#include <QString>
#include <QVariant>

struct Data_Configuration
{
	QStringList header_titles;
	QStringList what_to_collect;
	QString sql_table;
	QStringList raw_data_columns;
	QString raw_data_table;
	QString sorting_strategy;
	int columns_to_show;
};

using Metadata = std::vector<QVariant>;
using Labeled_Metadata = std::map<QString, QVariant>;

template<typename FloatType>
constexpr QVector<FloatType> toQVec( const std::vector<FloatType> & input )
{
	return QVector<FloatType>::fromStdVector( input );
}

inline QVector<double> toQVec( const arma::vec & input )
{
	return toQVec( arma::conv_to<std::vector<double>>::from( input ) );
}

template< typename T >
T Info_Or_Default( const Labeled_Metadata & meta, QString column, T Default )
{
	auto stuff = meta.find( column );
	if( stuff == meta.end() || stuff->second == QVariant::Invalid )
		return Default;
	return qvariant_cast<T>( stuff->second );
}

#include "rangeless_helper.hpp"
inline Labeled_Metadata Label_Metadata( Metadata meta, QStringList labels )
{
	return fn::zip( meta, labels )
		% fn::transform( []( const auto & x ) { auto &[ m, label ] = x; return std::pair<QString, QVariant>{label, m}; } )
		% fn::to( Labeled_Metadata{} );
}
