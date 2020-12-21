#pragma once

#include <tuple>

#include "fn.hpp"


namespace rangeless::fn
{

template<typename Iterable1, typename Iterable2>
struct zip_gen
{
	Iterable1 src1;
	Iterable2 src2;
	typename Iterable1::iterator it1;
	typename Iterable2::iterator it2;
	bool started;

	//using value_type = decltype( { std::move( *it1 ), std::move( *it2 ) } );
	using value_type = std::tuple< decltype( *it1 ), decltype( *it2 ) >;

	auto operator()() -> impl::maybe<value_type>
	{
		if( !started )
		{
			it1 = src1.begin();
			it2 = src2.begin();
			started = true;
		}

		if( it1 == src1.end() || it2 == src2.end() )
		{
			return { };
		}

		auto ret = value_type{ *it1, *it2 };
		++it1;
		++it2;
		return { ret };
	}
};

template<typename Iterable1, typename Iterable2>
impl::seq<zip_gen<Iterable1, Iterable2>> zip( Iterable1 src1, Iterable2 src2 )
{
	return { { std::move( src1 ), std::move( src2 ), {}, {}, false } };
}

//struct transpose2D
//{
//	template<typename IterableOfIterables>
//	struct gen
//	{
//		IterableOfIterables src1;
//		typename IterableOfIterables::iterator it1;
//		using Iterable = decltype( *it1 );
//		Iterable iterable;
//		typename Iterable::iterator it2;
//		using value_type = decltype( *it2 );
//		std::vector< decltype( it2 ) > iterator_per_row;
//		bool started;
//
//		using return_value_type = std::vector< value_type >;
//
//		auto operator()() -> impl::maybe<return_value_type>
//		{
//			if( !started )
//			{
//				iterator_per_row.resize( src1.size() );
//				for( size_t i = 0; i < src1.size(); i++ )
//				{
//					iterator_per_row[ i ] = src1[ i ].begin();
//				}
//				started = true;
//			}
//
//
//			return_value_type ret( src1.size() );
//			bool something_left = false;
//			for( size_t i = 0; i < src1.size(); i++ )
//			{
//				if( iterator_per_row[ i ] == src1[ i ].end() )
//					ret[ i ] = {};
//				else
//				{
//					ret[ i ] = *( iterator_per_row[ i ] );
//					iterator_per_row[ i ]++;
//					something_left = true;
//				}
//			}
//
//			if( something_left )
//				return { std::move( ret ) };
//			else
//				return {};
//		}
//	};
//
//	template<typename IterableOfIterables>
//	auto operator()( IterableOfIterables src1 ) && -> impl::seq<gen<IterableOfIterables>> // rvalue-specific because src2 is moved-from
//	{
//		return { { std::move( src1 ), {}, {}, false } };
//	}
//};

//template <class... Ts> struct tuple
//{
//};
//
//template <class T, class... Ts>
//struct tuple<T, Ts...> : tuple<Ts...>
//{
//	tuple( T t, Ts... ts ) : tuple<Ts...>( ts... ), tail( t )
//	{
//	}
//
//	T tail;
//};
//auto zip = []
//{
//	return fn::adapt( []( auto gen1, auto gen2 ) mutable
//	{
//		if( !gen1 || !gen2 )
//			return fn::end_seq();
//
//		return std::make_tuple{ gen1(), gen2() };
//	} );
//};
}