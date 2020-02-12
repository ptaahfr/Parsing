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
        using LAST_MEMBER::GetMemberInfo;
    };
}

namespace std
{
    template <size_t INDEX, typename LAST_MEMBER>
    auto & get(NamedTuple::NamedTuple<LAST_MEMBER> & namedTuple)
    {
        static_assert(INDEX < LAST_MEMBER::Count, "Index out of bounds");
        return namedTuple.Get(std::integral_constant<size_t, INDEX>());
    }

    template <size_t INDEX, typename LAST_MEMBER>
    auto const & get(NamedTuple::NamedTuple<LAST_MEMBER> const & namedTuple)
    {
        static_assert(INDEX < LAST_MEMBER::Count, "Index out of bounds");
        return namedTuple.Get(std::integral_constant<size_t, INDEX>());
    }

    template <typename LAST_MEMBER>
    struct tuple_size<NamedTuple::NamedTuple<LAST_MEMBER> > : public std::integral_constant<size_t, LAST_MEMBER::Count>
    {
    };
}

namespace NamedTuple
{
#define NAMEDTUPLE_BEGIN(name)                                               \
namespace name##Members                                                      \
{                                                                            \
    template <int ID>                                                        \
    class Member                                                             \
    {                                                                        \
    public:                                                                  \
        enum { Count = 0 };                                                  \
        template <size_t INDEX>                                              \
        inline std::nullptr_t Get(std::integral_constant<size_t, INDEX>) const    \
        {                                                                    \
            return nullptr;                                                  \
        }                                                                    \
        template <size_t INDEX>                                              \
        inline std::nullptr_t GetName(std::integral_constant<size_t, INDEX>) const    \
        {                                                                    \
            return nullptr;                                                  \
        }                                                                    \
        template <size_t INDEX>                                              \
        inline Member<ID> const & GetMemberInfo(std::integral_constant<size_t, INDEX>) const { return *this; } \
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
        using Base::GetMemberInfo;                                                                                           \
        std::integral_constant<size_t, Index> GetIndex() const { return std::integral_constant<size_t, Index>(); } \
        std::integral_constant<size_t, Count> GetCount() const { return std::integral_constant<size_t, Count>(); } \
        Member<ID - 1> const & GetParent() const { return *this; }                                                 \
        Member<ID - 1> & GetParent() { return *this; }                                                             \
        inline char const * GetName(std::integral_constant<size_t, Index>) const { return #member; }               \
        inline char const * GetName() const { return #member; }               \
        template <size_t INDEX> \
        inline char const * GetName(std::integral_constant<size_t, INDEX> index) const { return Base::GetName(index); }               \
        inline type & Get(std::integral_constant<size_t, Index>)                                                   \
        {                                                                                                          \
            return member;                                                                                         \
        }                                                                                                          \
        inline type const & Get(std::integral_constant<size_t, Index>) const                                       \
        {                                                                                                          \
            return member;                                                                                         \
        }                                                                                                          \
        inline Member<ID> & GetMemberInfo(std::integral_constant<size_t, Index>)                                   \
        {                                                                                                          \
            return *this;                                                                                          \
        }                                                                                                          \
        inline Member<ID> const & GetMemberInfo(std::integral_constant<size_t, Index>) const                       \
        {                                                                                                          \
            return *this;                                                                                          \
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

    //template <typename OSTREAM, size_t FIRST_INDEX = 0, typename LAST_MEMBER, typename ACTION, typename ... RESOLVE_ARGS>
    //static inline OSTREAM & WriteNamedTuple(std::integral_constant<size_t, FIRST_INDEX> firstIndex, size_t indentation,
    //    OSTREAM & os, NamedTuple<LAST_MEMBER> const & namedTuple, ACTION && action,
    //    RESOLVE_ARGS && ... resolveArgs)
    //{
    //    if (indentation > 0)
    //    {
    //        os << std::string(indentation, ' ');
    //    }

    //    os << action(namedTuple.Get(firstIndex), std::forward<RESOLVE_ARGS>(resolveArgs)...) << std::endl;

    //    enum { NEXT_INDEX = __min(FIRST_INDEX + 1, NamedTuple<LAST_MEMBER>::Count - 1) };
    //    if (NEXT_INDEX > FIRST_INDEX)
    //    {
    //        return WriteNamedTuple(std::integral_constant<size_t, NEXT_INDEX>(), os, namedTuple, std::forward(action), std::forward<RESOLVE_ARGS>(resolveArgs)...);
    //    }
    //    else
    //    {
    //        return os;
    //    }
    //}

    //template <typename OSTREAM, typename LAST_MEMBER, typename ACTION, typename ... RESOLVE_ARGS>
    //static inline OSTREAM & WriteNamedTuple(OSTREAM & os, NamedTuple<LAST_MEMBER> const & namedTuple,
    //    ACTION && action, RESOLVE_ARGS && ... resolveArgs)
    //{
    //    return WriteNamedTuple(std::integral_constant<size_t, 0>(), 0, os, namedTuple, std::forward(action), std::forward<RESOLVE_ARGS>(resolveArgs)...);
    //}

    template <typename TYPE>
    class IsTuplish : public std::false_type { };

    template <typename... ARGS>
    class IsTuplish<std::tuple<ARGS...> > : public std::true_type { };

    template <typename LAST_MEMBER>
    class IsTuplish<::NamedTuple::NamedTuple<LAST_MEMBER> > : public std::true_type { };

#define ENABLED_IF_TUPLISH(which) std::enable_if_t<::NamedTuple::IsTuplish<which>::value, void *> = nullptr
#define ENABLED_IF_NOT_TUPLISH(which) std::enable_if_t<!::NamedTuple::IsTuplish<which>::value, void *> = nullptr
#define ENABLED_IF_TUPLISH_DEF(which) std::enable_if_t<::NamedTuple::IsTuplish<which>::value, void *> 

    class Visitor
    {
    public:
        template <typename MEMBER_INFO, typename TYPE, typename CONTINUATION>
        inline void OnMember(MEMBER_INFO const & memberInfo, TYPE const & memberValue, CONTINUATION && continuation)
        {
            continuation();
        }
    };


    template <typename MEMBER_INFO, typename MEMBER_VALUE, typename VISITOR>
    inline void VisitNode(MEMBER_INFO const & memberInfo, MEMBER_VALUE const & memberValue, VISITOR && visitor)
    {
        visitor.OnMember(memberInfo, memberValue, [] { });
    }

    template <typename MEMBER_INFO, typename TYPE, typename VISITOR>
    inline void VisitNode(MEMBER_INFO const & memberInfo, std::vector<TYPE> const & memberValue, VISITOR && visitor);

    template <typename MEMBER_INFO, typename LAST_MEMBER, typename VISITOR>
    inline void VisitNode(MEMBER_INFO const & memberInfo, NamedTuple<LAST_MEMBER> const & that, VISITOR && visitor);

    template <typename MEMBER_INFO, typename TYPE, typename VISITOR>
    inline void VisitNode(MEMBER_INFO const & memberInfo, std::vector<TYPE> const & that, VISITOR && visitor)
    {
        visitor.OnMember(memberInfo, that, [&]
        {
            for (TYPE const & item : that)
            {
                VisitNode(memberInfo, item, std::forward<VISITOR>(visitor));
            }
        });
    }

    template <typename LAST_MEMBER, typename VISITOR, size_t OFFSET>
    inline void VisitItems(NamedTuple<LAST_MEMBER> const & that, VISITOR && visitor, std::integral_constant<size_t, OFFSET> offset)
    {
        enum { NEXT_OFFSET = __min(OFFSET + 1, NamedTuple<LAST_MEMBER>::Count - 1) };

        VisitNode(that.GetMemberInfo(offset), that.Get(offset), std::forward<VISITOR>(visitor));

        if (OFFSET < NEXT_OFFSET)
            VisitItems(that, std::forward<VISITOR>(visitor), std::integral_constant<size_t, NEXT_OFFSET>());
    }

    template <typename MEMBER_INFO, typename LAST_MEMBER, typename VISITOR>
    inline void VisitNode(MEMBER_INFO const & memberInfo, NamedTuple<LAST_MEMBER> const & that, VISITOR && visitor)
    {
        visitor.OnMember(memberInfo, that, [&]
        {
            VisitItems(that, std::forward<VISITOR>(visitor), std::integral_constant<size_t, 0>());
        });
    }

    template <typename LAST_MEMBER, typename VISITOR>
    inline void Visit(NamedTuple<LAST_MEMBER> const & that, VISITOR && visitor)
    {
        VisitNode(that, that, std::forward<VISITOR>(visitor));
    }
}
