// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test named tuples

#pragma once

#include <cstddef>
#include <type_traits>
#include <string>
#include <tuple>
#include <sstream>

namespace NamedTuple
{
    template <typename LAST_MEMBER>
    class NamedTuple : public LAST_MEMBER
    {
    public:
        enum { Count = LAST_MEMBER::Count };
        using LAST_MEMBER::Get;
    };

    template <size_t INDEX, typename LAST_MEMBER>
    auto & get(NamedTuple<LAST_MEMBER> & namedTuple)
    {
        static_assert(INDEX < LAST_MEMBER::Count, "Index out of bounds");
        return namedTuple.Get(std::integral_constant<size_t, INDEX>());
    }

    template <size_t INDEX, typename LAST_MEMBER>
    auto const & get(NamedTuple<LAST_MEMBER> const & namedTuple)
    {
        static_assert(INDEX < LAST_MEMBER::Count, "Index out of bounds");
        return namedTuple.Get(std::integral_constant<size_t, INDEX>());
    }

    template <typename LAST_MEMBER>
    class ::std::tuple_size<NamedTuple<LAST_MEMBER> > : public std::integral_constant<size_t, LAST_MEMBER::Count>
    {
    };
}

namespace std
{
    using NamedTuple::get;
}

#define NAMEDTUPLE_BEGIN(name)                                               \
namespace name##Members                                                      \
{                                                                            \
    template <int ID>                                                        \
    class Member                                                             \
    {                                                                        \
    public:                                                                  \
        enum { Count = 0 };                                                  \
        template <size_t INDEX>                                              \
        inline nullptr_t Get(std::integral_constant<size_t, INDEX>) const    \
        {                                                                    \
            return nullptr;                                                  \
        }                                                                    \
        inline std::tuple<> AsTuple() const { return std::make_tuple();  }   \
        inline std::tuple<> AsRTuple() const { return std::make_tuple();  }  \
        inline std::tuple<> AsRCTuple() const { return std::make_tuple();  } \
        inline void Reset() { }                                              \
    };

#define NAMEDTUPLE_ITEM_INTERNAL(ID, type, member, value, ...)                                                     \
    template <>                                                                                                    \
    class Member<ID> : public Member<ID - 1>                                                                       \
    {                                                                                                              \
        enum { Id = ID };                                                                                          \
        using Base = Member<ID - 1>;                                                                               \
    public:                                                                                                        \
        enum { Count = Base::Count + 1 };                                                                          \
        enum { Index = Base::Count };                                                                              \
        type member = { value };                                                                                   \
        using Base::Get;                                                                                           \
        inline type & Get(std::integral_constant<size_t, Index>)                                                   \
        {                                                                                                          \
            return member;                                                                                         \
        }                                                                                                          \
        inline type const & Get(std::integral_constant<size_t, Index>) const                                       \
        {                                                                                                          \
            return member;                                                                                         \
        }                                                                                                          \
        inline auto AsTuple() const { return std::tuple_cat(Base::AsTuple(), std::make_tuple((type)member));  }    \
        inline auto AsRTuple() { return std::tuple_cat(Base::AsRTuple(), std::tie((type &)member));  }             \
        inline auto AsRTuple() const { return std::tuple_cat(Base::AsRTuple(), std::tie((type const &)member));  } \
        inline auto AsRCTuple() const { return AsRTuple();  }                                                      \
        inline void Reset() { member.~type(); new (&member) type({ value }); Base::Reset(); }                      \
    };

#define NAMEDTUPLE_ITEM(type, member, value, ...) \
    NAMEDTUPLE_ITEM_INTERNAL(__COUNTER__, type, member, value, __VA_ARGS__)

#define NAMEDTUPLE_END(name)                                                   \
}                                                                              \
using name = NamedTuple::NamedTuple<name##Members::Member<__COUNTER__ - 1> >;
