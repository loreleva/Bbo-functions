
#pragma once

#include <stdexcept>


//
// Simple variant type that can be used with classes (in contrast to union).
// Example usage:
//    Variant<int, double, vector<double>, string>
//
// It makes the following assumptions:
// (1) the first type must be default constructible,
// (2) all types in the collection must be copy constructible.
// (3) all types are default destructible.
// (4) no type throws in its constructor or destructor.
// As a result the variant type is default constructible, copy
// constructible, and default destructible, and it does not throw
// exceptions during construction or destruction.
//


namespace detail {

// This type is the Variant's default argument type.
struct NoArgument2 { };
struct NoArgument3 { };
struct NoArgument4 { };
struct NoArgument5 { };
struct NoArgument6 { };
struct NoArgument7 { };

// template meta programming
struct _true { };
struct _false { };

template <typename T>
struct ArgumentTraits
{
	typedef _true type;
	static const size_t num = 1;
};

template <> struct ArgumentTraits<NoArgument2> { typedef _false type; static const size_t num = 0; };
template <> struct ArgumentTraits<NoArgument3> { typedef _false type; static const size_t num = 0; };
template <> struct ArgumentTraits<NoArgument4> { typedef _false type; static const size_t num = 0; };
template <> struct ArgumentTraits<NoArgument5> { typedef _false type; static const size_t num = 0; };
template <> struct ArgumentTraits<NoArgument6> { typedef _false type; static const size_t num = 0; };
template <> struct ArgumentTraits<NoArgument7> { typedef _false type; static const size_t num = 0; };

// This type acts as the end iterator of any sequence.
struct EmptySequence {
	static const size_t size = 0;
	static const size_t storage_size = 0;
};

// A sequence represents several things, namely:
// (*) A collection of types.
// (*) An iterator into a sequence of types.
// The first argument is a plain type to be stored.
// In contrast, the second argument is in itself a sequence!
template <typename FIRST, typename REST>
struct Sequence
{
	typedef FIRST first_type;
	typedef REST rest_type;

	static const size_t size = REST::size + ArgumentTraits<FIRST>::num;
	static const size_t storage_size = (sizeof(FIRST) >= REST::storage_size) ? sizeof(FIRST) : REST::storage_size;
};

// move forward in sequence
template <typename SEQUENCE, int INDEX>
struct forward
{
	typedef typename forward<typename SEQUENCE::rest_type, INDEX - 1>::type type;
};

// move forward in sequence
template <typename SEQUENCE>
struct forward<SEQUENCE, 0>
{
	typedef SEQUENCE type;
};

// access type in sequence
template <typename SEQUENCE, int INDEX>
struct at
{
	typedef typename forward<SEQUENCE, INDEX>::type ElemType;
	typedef typename ElemType::first_type type;
};

// check whether an element is contained in the sequence

// return index of element in sequence
template <typename ELEMENT, typename SEQUENCE>
struct index_of
{
	static const int index = index_of<ELEMENT, typename SEQUENCE::rest_type>::index + 1;
};

template <typename ELEMENT, typename REST>
struct index_of<ELEMENT, Sequence<ELEMENT, REST> >
{
	static const int index = 0;
};

};


#define VARIANT_PER_TYPE(N, T) \
public: \
	Variant(T const& value) \
	: m_type(N) \
	{ \
		new (m_storage) T(value); \
	} \
\
	T const& operator = (T const& other) \
	{ \
		if ((void*)m_storage == (void*)(&other)) return other; \
		if (m_type == N) get< N >() = other; \
		else \
		{ \
			invalidate(); \
			new (m_storage) T(other); \
			m_type = N; \
		} \
		return other; \
	}




template<
	typename T0,
	typename T1,
	typename T2 = detail::NoArgument2,
	typename T3 = detail::NoArgument3,
	typename T4 = detail::NoArgument4,
	typename T5 = detail::NoArgument5,
	typename T6 = detail::NoArgument6,
	typename T7 = detail::NoArgument7
>
class Variant
{
private:
	typedef
			detail::Sequence<T0,
				detail::Sequence<T1,
					detail::Sequence<T2,
						detail::Sequence<T3,
							detail::Sequence<T4,
								detail::Sequence<T5,
									detail::Sequence<T6,
										detail::Sequence<T7,
											detail::EmptySequence
										>
									>
								>
							>
						>
					>
				>
			>
		SequenceType;

	char m_storage[SequenceType::storage_size];
	unsigned int m_type;

	template <int INDEX>
	typename detail::at<SequenceType, INDEX>::type* ptr()
	{
		if (m_type != INDEX) throw std::runtime_error("[variant] attempt to access data as wrong type"); \
		typedef typename detail::at<SequenceType, INDEX>::type ReturnType;
		return ((ReturnType*)(void*)(&m_storage));
	}

	template <int INDEX>
	const typename detail::at<SequenceType, INDEX>::type* const_ptr() const
	{
		if (m_type != INDEX) throw std::runtime_error("[variant] attempt to access data as wrong type"); \
		typedef typename detail::at<SequenceType, INDEX>::type ReturnType;
		return ((const ReturnType*)(void*)(&m_storage));
	}

	// in-place destruct object - afterwards state is undefined
	void invalidate()
	{
		if (SequenceType::size <= 4 || m_type < 4)
		{
			if (SequenceType::size <= 2 || m_type < 2)
			{
				if (SequenceType::size <= 1 || m_type < 1) ptr<0>()->~T0();
				else ptr<1>()->~T1();
			}
			else
			{
				if (SequenceType::size <= 3 || m_type < 3) ptr<2>()->~T2();
				else ptr<3>()->~T3();
			}
		}
		else
		{
			if (SequenceType::size <= 6 || m_type < 6)
			{
				if (SequenceType::size <= 5 || m_type < 5) ptr<4>()->~T4();
				else ptr<5>()->~T5();
			}
			else
			{
				if (SequenceType::size <= 7 || m_type < 7) ptr<6>()->~T6();
				else ptr<7>()->~T7();
			}
		}
	}

	// in-place construct - assuming that the state was undefined before
	void create(Variant const& other)
	{
		if (SequenceType::size <= 4 || m_type < 4)
		{
			if (SequenceType::size <= 2 || m_type < 2)
			{
				if (SequenceType::size <= 1 || m_type < 1) new (m_storage) T0(other.get<0>());
				else new (m_storage) T1(other.get<1>());
			}
			else
			{
				if (SequenceType::size <= 3 || m_type < 3) new (m_storage) T2(other.get<2>());
				else new (m_storage) T3(other.get<3>());
			}
		}
		else
		{
			if (SequenceType::size <= 6 || m_type < 6)
			{
				if (SequenceType::size <= 5 || m_type < 5) new (m_storage) T4(other.get<4>());
				else new (m_storage) T5(other.get<5>());
			}
			else
			{
				if (SequenceType::size <= 7 || m_type < 7) new (m_storage) T6(other.get<6>());
				else new (m_storage) T7(other.get<7>());
			}
		}
	}

public:
	// (zero-based) index of the current type
	// or (unsigned int)-1 in case of undefined state
	unsigned int type() const
	{ return m_type; }

	// data access by index
	template <int INDEX>
	typename detail::at<SequenceType, INDEX>::type& get()
	{ return *(ptr<INDEX>()); }

	template <int INDEX>
	typename detail::at<SequenceType, INDEX>::type const& get() const
	{ return *(const_ptr<INDEX>()); }

	// data access by type
	template <typename T>
	typename detail::at<SequenceType, detail::index_of<T, SequenceType>::index>::type& as()
	{ return *(ptr<detail::index_of<T, SequenceType>::index>()); }

	template <typename T>
	typename detail::at<SequenceType, detail::index_of<T, SequenceType>::index>::type const& as() const
	{ return *(const_ptr<detail::index_of<T, SequenceType>::index>()); }

	Variant()
	: m_type(0)
	{
		new (m_storage) T0();
	}

	~Variant()
	{ invalidate(); }

	Variant(Variant const& other)
	: m_type(other.m_type)
	{
		create(other);
	}

	Variant& operator = (Variant& other)
	{
		if (this == &other) return other;
		if (m_type == other.m_type)
		{
			if (SequenceType::size <= 4 || m_type < 4)
			{
				if (SequenceType::size <= 2 || m_type < 2)
				{
					if (SequenceType::size <= 1 || m_type < 1) get<0>() = other.get<0>();
					else get<1>() = other.get<1>();
				}
				else
				{
					if (SequenceType::size <= 3 || m_type < 3) get<2>() = other.get<2>();
					else get<3>() = other.get<3>();
				}
			}
			else
			{
				if (SequenceType::size <= 6 || m_type < 6)
				{
					if (SequenceType::size <= 5 || m_type < 5) get<4>() = other.get<4>();
					else get<5>() = other.get<5>();
				}
				else
				{
					if (SequenceType::size <= 7 || m_type < 7) get<6>() = other.get<6>();
					else get<7>() = other.get<7>();
				}
			}
		}
		else
		{
			invalidate();
			m_type = other.m_type;
			create(other);
		}
		return other;
	}

	VARIANT_PER_TYPE(0, T0)
	VARIANT_PER_TYPE(1, T1)
	VARIANT_PER_TYPE(2, T2)
	VARIANT_PER_TYPE(3, T3)
	VARIANT_PER_TYPE(4, T4)
	VARIANT_PER_TYPE(5, T5)
	VARIANT_PER_TYPE(6, T6)
	VARIANT_PER_TYPE(7, T7)
};
