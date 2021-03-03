#pragma once

#include <tuple>
#include <armadillo>

#include "fn.hpp"

// Adapted From: https://stackoverflow.com/questions/87372/check-if-a-class-has-a-member-function-of-a-given-signature
#include <type_traits>

// Primary template with a static assertion
// for a meaningful error message
// if it ever gets instantiated.
// We could leave it undefined if we didn't care.

template<typename, typename T>
struct has_emplace_back
{
	static_assert(
		std::integral_constant<T, false>::value,
		"Second template parameter needs to be of function type." );
};

// specialization that does the checking

template<typename C, typename Ret, typename... Args>
struct has_emplace_back<C, Ret( Args... )>
{
private:
	template<typename T>
	static constexpr auto check( T* )
		-> typename
		std::is_same<
		decltype( std::declval<T>().emplace_back( std::declval<Args>()... ) ),
		Ret    // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		>::type;  // attempt to call it and see if the return type is correct

	template<typename>
	static constexpr std::false_type check( ... );

	typedef decltype( check<C>( 0 ) ) type;

public:
	static constexpr bool value = type::value;
};


namespace rangeless::fn
{

// Based on https://stackoverflow.com/questions/40781417/declare-member-variables-from-variadic-template-parameter/53075340#53075340
template <class T, class... Rest> class hold : hold<Rest...>
{
	using base = hold<Rest...>;
	T v_;

public:
	constexpr hold( T v, Rest... a ) : base( a... ), v_( v )
	{
	}

	template <class F, class... args>
	constexpr auto apply( F f, args & ... a )
	{
		return base::apply( f, a..., v_ );
	}

	constexpr bool any_equal( const hold<T, Rest...> & rhs )
	{
		return v_ == rhs.v_ || base::any_equal( static_cast<const base &>( rhs ) );
	}
};

template <class T> class hold<T>
{
	T v_;

public:
	constexpr hold( T v ) : v_( v )
	{
	}

	template <class F, class... args>
	constexpr auto apply( F f, args & ... a )
	{
		return f( a..., v_ );
	}

	constexpr bool any_equal( const hold<T> & rhs )
	{
		return v_ == rhs.v_;
	}
};

template <typename... Ts>
constexpr void variadic_expand( const Ts&...args )
{
}

template <class... Iterators>
struct zip_gen
{
	hold<Iterators...> iterators;
	const hold<Iterators...> end_iterators;

	using value_type = typename std::tuple< std::add_lvalue_reference_t< std::remove_reference_t<decltype( *std::declval<Iterators>() )> >... >;

	constexpr zip_gen( Iterators... begin, Iterators... end ) : iterators( begin... ), end_iterators( end... )
	{
	}

	constexpr auto operator()() -> rangeless::fn::impl::maybe<value_type>
	{
		if( iterators.any_equal( end_iterators ) )
		{
			return { };
		}

		value_type && ret = iterators.apply( []( Iterators & ... iters ) { return std::tie( *iters... ); } );
		iterators.apply( []( Iterators & ... iters ) { variadic_expand( ++iters... ); } );
		return { std::move( ret ) };
	}

};

template<typename ...Iterables>
constexpr auto zip( Iterables& ... src ) noexcept
{
	using zip_gen_type = zip_gen< std::remove_reference_t<decltype( std::begin( src ) )>... >;

	return rangeless::fn::impl::seq < zip_gen_type >{
		{
			std::begin( src )..., std::end( src )...
		} };
}


//template<typename Iterable1, typename Iterable2>
//struct unzip_gen
//{
//	Iterable1 src1;
//	Iterable2 src2;
//	typename Iterable1::iterator it1;
//	typename Iterable2::iterator it2;
//
//	//using value_type = decltype( { std::move( *it1 ), std::move( *it2 ) } );
//	using value_type = std::tuple< decltype( *it1 ), decltype( *it2 ) >;
//
//	auto operator()( std::tuple<Iterable1, Iterable2> ) -> impl::maybe<value_type>
//	{
//		if( it1 == src1.end() || it2 == src2.end() )
//		{
//			return { };
//		}
//
//		auto ret = value_type{ *it1, *it2 };
//		++it1;
//		++it2;
//		return { ret };
//	}
//};
//
//template<typename Iterable1, typename Iterable2>
//impl::seq<unzip_gen<Iterable1, Iterable2>> unzip( std::tuple<Iterable1, Iterable2> )
//{
//	return { { std::move( src1 ), std::move( src2 ), src1.begin(), src2.begin() } };
//}

//template<typename Iterable, typename value_type = Iterable::value_type>
//struct unzip_gen_helper
//{
//	Iterable storage;
//	void emplace_back( Iterable::value_type && v )
//	{
//		storage.emplace_back( v );
//	}
//
//	Iterable results()
//	{
//		return std::move( storage );
//	}
//};
//
//template<typename value_type>
//struct unzip_gen_helper<arma::vec, value_type>
//{
//	std::vector< value_type > storage;
//	void emplace_back( Iterable::value_type && v )
//	{
//		storage.emplace_back( v );
//	}
//
//	Iterable results()
//	{
//		return std::move( arma::conv_to<arma::vec>::from( storage ) );
//	}
//};
//
//template< typename T, typename... Iterables >
//struct unzip_splitter : unzip_splitter< Iterables... >
//{
//	unzip_gen_helper<T> storage;
//	template< int index >
//	void emplace_back( auto && x )
//	{
//		storage.emplace_back( std::get< index >( x ) );
//	}
//
//	std::tuple< T, Iterables... > split()
//	{
//		return { storage, };
//	}
//};
//
//template<typename... Iterables>
//struct unzip_gen_n
//{
//	// passthrough overload
//	std::tuple<Iterables...> operator()( std::tuple<Iterables...> tup ) const
//	{
//		return std::move( tup );
//	}
//
//	// overload for a seq - invoke rvalue-specific implicit conversion
//	template<typename Gen,
//		typename Out_Type = std::tuple<Iterables...> >
//		Out_Type operator()( impl::seq<Gen> r ) const
//	{
//		unzip_splitter<Iterables...> out;
//		for( auto && x : r )
//			out.emplace_back( std::move( x ) );
//
//		return out.split();
//	}
//};

template<typename Iterable1, typename Iterable2>
struct unzip_gen
{
	// passthrough overload
	std::tuple<Iterable1, Iterable2> operator()( std::tuple<Iterable1, Iterable2> tup ) const
	{
		return std::move( tup );
	}

	// overload for a seq - invoke rvalue-specific implicit conversion
	template<typename Gen,
		typename Out_Type = std::tuple<Iterable1, Iterable2> >
	Out_Type operator()( impl::seq<Gen> r ) const
	{
		Iterable1 out1;
		std::vector< Iterable1::value_type > temp1;
		Iterable2 out2;
		std::vector< Iterable2::value_type > temp2;
		for( auto && x : r )
		{
			if constexpr( has_emplace_back< Iterable1, Iterable1::value_type( Iterable1::value_type ) >::value )
				out1.emplace_back( std::move( std::get<0>( x ) ) );
			else
				temp1.emplace_back( std::move( std::get<0>( x ) ) );

			if constexpr( has_emplace_back< Iterable2, Iterable2::value_type( Iterable2::value_type ) >::value )
				out2.emplace_back( std::move( std::get<1>( x ) ) );
			else
				temp2.emplace_back( std::move( std::get<1>( x ) ) );
		}
		if constexpr( !has_emplace_back< Iterable1, Iterable1::value_type( Iterable1::value_type ) >::value )
			out1 = arma::conv_to<Iterable1>::from( temp1 );
		if constexpr( !has_emplace_back< Iterable2, Iterable2::value_type( Iterable2::value_type ) >::value )
			out2 = arma::conv_to<Iterable1>::from( temp2 );

		return Out_Type{ std::move( out1 ), std::move( out2 ) };
	}

	//// overload for other iterable: move-insert elements into vec
	//template<typename Iterable,
	//	typename Vec = std::vector<typename Iterable::value_type> >
	//	Vec operator()( Iterable src ) const
	//{
	//	// Note: this will not compile with std::set
	//	// in conjunction with move-only value_type because 
	//	// set's iterators are const, and std::move will try 
	//	// and fail to use the copy-constructor.

	//	return this->operator()( Vec{
	//			std::make_move_iterator( src.begin() ),
	//			std::make_move_iterator( src.end() ) } );
	//}
};

template<typename Iterable1, typename Iterable2>
unzip_gen< Iterable1, Iterable2 > unzip( std::tuple<Iterable1, Iterable2> )
{
	return {};
}

//template< typename Iterator, std::enable_if_t<std::is_pointer_v<Iterator>, bool> = true >
//class view
//{
//private:
//	Iterator it_beg;
//	Iterator it_end;
//
//public:
//	view( Iterator b, Iterator e )
//		: it_beg( std::move( b ) )
//		, it_end( std::move( e ) )
//	{
//	}
//
//	view() = default;
//
//	using iterator = Iterator;
//	//using value_type = typename iterator::value_type;
//
//	Iterator begin() const
//	{
//		return it_beg;
//	}
//	Iterator end()   const
//	{
//		return it_end;
//	}
//
//	///// Truncate the view.
//	/////
//	///// Precondition: `b == begin() || e = end()`; throws `std::logic_error` otherwise.
//	///// This does not affect the underlying range.
//	//void erase( Iterator b, Iterator e )
//	//{
//	//	// We support the erase method to obviate view-specific overloads 
//	//	// for some hofs, e.g. take_while, take_first, drop_whille, drop_last, etc -
//	//	// the container-specific overloads will work for views as well.
//
//	//	if( b == it_beg )
//	//	{
//	//		// erase at front
//	//		it_beg = e;
//	//	}
//	//	else if( e == it_end )
//	//	{
//
//	//		// erase at end
//	//		impl::require_iterator_category_at_least<std::forward_iterator_tag>( *this );
//
//	//		it_end = b;
//	//	}
//	//	else
//	//	{
//	//		RANGELESS_FN_THROW( "Can only erase at the head or at the tail of the view" );
//	//	}
//	//}
//
//	void clear()
//	{
//		it_beg = it_end;
//	}
//
//	bool empty() const
//	{
//		return it_beg == it_end;
//	}
//};

//template<typename From_Type, typename Interior_Type>
//std::vector<Interior_Type> from_arma( From_Type< Interior_Type > f )
//{
//	return conv_to< std::vector<Interior_Type> >( std::move( f ) );
//}

//template<typename Interior_Type, typename< template<typename...> class From_Type>
//struct to_arma
//{
//	auto operator()( From_Type< Interior_Type > f )
//	{
//		return conv_to< std::vector<Interior_Type> >( std::move( f ) );
//	}
//};

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

namespace fn = rangeless::fn;
using fn::operators::operator%;   // arg % f % g % h; // h(g(f(std::forward<Arg>(arg))));
