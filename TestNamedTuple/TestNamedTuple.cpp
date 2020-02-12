// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test named tuples

#include <cstddef>
#include <type_traits>
#include <string>
#include <tuple>
#include <sstream>

#define VERSION 4

#if VERSION == 0

struct DataTestStruct
{
    std::string Field1;
    std::string Field2;
};

#elif VERSION == 1

#define NAMEDTUPLE_BEGIN(name) \
namespace name##Members        \
{                              \
template <int ID>              \
class Member                   \
{                              \
public:                        \
    enum { Count = 0 };        \
};

#define NAMEDTUPLE_ITEM_INTERNAL(ID, type, member, value) \
template <>                                               \
class Member<ID> : public Member<ID - 1>                  \
{                                                         \
    enum { Id = ID };                                     \
public:                                                   \
    enum { Count = Member<ID - 1>::Count + 1 };           \
    type member = { value };                              \
};                                                        \

#define NAMEDTUPLE_ITEM(type, member, value) \
    NAMEDTUPLE_ITEM_INTERNAL(__COUNTER__, type, member, value)

#define NAMEDTUPLE_END(name)                         \
}                                                    \
using name = name##Members::Member<__COUNTER__ - 1>;

#elif VERSION == 2

#define NAMEDTUPLE_BEGIN(name)                                      \
namespace name##Members                                             \
{                                                                   \
    template <int ID>                                               \
    class Member                                                    \
    {                                                               \
    public:                                                         \
        enum { Count = 0 };                                         \
        template <size_t INDEX>                                     \
        inline std::nullptr_t Get(std::integral_constant<size_t, INDEX>) const            \
        {                                                                            \
            return nullptr;                                                          \
        }                                                                            \
    };

#define NAMEDTUPLE_ITEM_INTERNAL(ID, type, member, value)                    \
    template <>                                                              \
    class Member<ID> : public Member<ID - 1>                                 \
    {                                                                        \
        enum { Id = ID };                                                    \
        using Base = Member<ID - 1>;                                         \
    public:                                                                  \
        enum { Index = Base::Count };                                        \
        enum { Count = Index + 1 };                                          \
        type member = { value };                                             \
        using Base::Get;                                                     \
        inline type & Get(std::integral_constant<size_t, Index>)             \
        {                                                                    \
            return member;                                                   \
        }                                                                    \
        inline type const & Get(std::integral_constant<size_t, Index>) const \
        {                                                                    \
            return member;                                                   \
        }                                                                    \
    };                                                                       \

#define NAMEDTUPLE_ITEM(type, member, value) \
    NAMEDTUPLE_ITEM_INTERNAL(__COUNTER__, type, member, value)

#define NAMEDTUPLE_END(name)                                                                          \
}                                                                                                     \
using name = name##Members::Member<__COUNTER__ - 1>;                                                  \
template <size_t INDEX>                                             \
auto get(name & namedTuple)                                         \
{                                                                   \
    static_assert(INDEX < name::Count, "Index out of bounds");      \
    return namedTuple.Get(std::integral_constant<size_t, INDEX>()); \
}                                                                   \
template <size_t INDEX>                                             \
auto get(name const& namedTuple)                                    \
{                                                                   \
    static_assert(INDEX < name::Count, "Index out of bounds");      \
    return namedTuple.Get(std::integral_constant<size_t, INDEX>()); \
}

#elif VERSION == 3

#define NAMEDTUPLE_BEGIN(name)                                              \
namespace name##Members                                                     \
{                                                                           \
    template <int ID>                                                       \
    class Member                                                            \
    {                                                                       \
    public:                                                                 \
        enum { Count = 0 };                                                 \
        template <size_t INDEX>                                             \
        inline std::nullptr_t Get(std::integral_constant<size_t, INDEX>) const            \
        {                                                                            \
            return nullptr;                                                          \
        }                                                                            \
        inline std::tuple<> AsTuple() const { return std::make_tuple();  }              \
        inline std::tuple<> AsRTuple() { return std::make_tuple();  }                  \
        inline std::tuple<> AsRTuple() const { return std::make_tuple();  }           \
        inline std::tuple<> AsRCTuple() const { return std::make_tuple();  }          \
    };

#define NAMEDTUPLE_ITEM_INTERNAL(ID, type, member, value)                                                                           \
    template <>                                                                                                                     \
    class Member<ID> : public Member<ID - 1>                                                                                        \
    {                                                                                                                               \
        enum { Id = ID };                                                                                                           \
        using Base = Member<ID - 1>;                                                                                                \
    public:                                                                                                                         \
        enum { Count = Base::Count + 1 };                                                                                           \
        enum { Index = Base::Count };                                                                                               \
        type member = { value };                                                                                                    \
        using Base::Get;                                                                                                            \
        inline type & Get(std::integral_constant<size_t, Index>)                                                                    \
        {                                                                                                                           \
            return member;                                                                                                          \
        }                                                                                                                           \
        inline type const & Get(std::integral_constant<size_t, Index>) const                                                        \
        {                                                                                                                           \
            return member;                                                                                                          \
        }                                                                                                                           \
        inline auto AsTuple() const { return std::tuple_cat(Base::AsTuple(), std::make_tuple((type)member));  }         \
        inline auto AsRTuple() { return std::tuple_cat(Base::AsRTuple(), std::tie((type &)member));  }                  \
        inline auto AsRTuple() const { return std::tuple_cat(Base::AsRTuple(), std::tie((type const &)member));  }      \
        inline auto AsRCTuple() const { return AsRTuple();  }                                                           \
    };

#define NAMEDTUPLE_ITEM(type, member, value) \
    NAMEDTUPLE_ITEM_INTERNAL(__COUNTER__, type, member, value)

#define NAMEDTUPLE_END(name)                                                                          \
}                                                                                                     \
using name = name##Members::Member<__COUNTER__ - 1>;                                                  \
template <size_t INDEX>                                             \
auto get(name & namedTuple)                                         \
{                                                                   \
    static_assert(INDEX < name::Count, "Index out of bounds");      \
    return namedTuple.Get(std::integral_constant<size_t, INDEX>()); \
}                                                                   \
template <size_t INDEX>                                             \
auto get(name const& namedTuple)                                    \
{                                                                   \
    static_assert(INDEX < name::Count, "Index out of bounds");      \
    return namedTuple.Get(std::integral_constant<size_t, INDEX>()); \
}

#elif VERSION == 4

#define NAMEDTUPLE_BEGIN(name)                                                       \
namespace name##Members                                                              \
{                                                                                    \
    template <int ID>                                                                \
    class Member                                                                     \
    {                                                                                \
    public:                                                                          \
        enum { Count = 0 };                                                          \
        template <size_t INDEX>                                                      \
        inline std::nullptr_t Get(std::integral_constant<size_t, INDEX>) const            \
        {                                                                            \
            return nullptr;                                                          \
        }                                                                            \
        inline std::tuple<> AsTuple() const { return std::make_tuple();  }              \
        inline std::tuple<> AsRTuple() { return std::make_tuple();  }                  \
        inline std::tuple<> AsRTuple() const { return std::make_tuple();  }           \
        inline std::tuple<> AsRCTuple() const { return std::make_tuple();  }          \
        inline char const * GetName(int index) const { return nullptr;  }            \
        inline char const * GetDescription(int index) const { return nullptr;  }     \
        inline int GetCount() const { return 0;  }                                   \
        inline std::string GetStringValue(int index) const { return std::string(); } \
        inline void Reset() { }                                                      \
    };

#define NAMEDTUPLE_ITEM_INTERNAL(ID, type, member, value, ...)                                                          \
    template <>                                                                                                         \
    class Member<ID> : public Member<ID - 1>                                                                            \
    {                                                                                                                   \
        enum { Id = ID };                                                                                               \
        using Base = Member<ID - 1>;                                                                                    \
    public:                                                                                                             \
        enum { Count = Base::Count + 1 };                                                                               \
        enum { Index = Base::Count };                                                                                   \
        type member = { value };                                                                                        \
        using Base::Get;                                                                                                \
        inline type & Get(std::integral_constant<size_t, Index>)                                                        \
        {                                                                                                               \
            return member;                                                                                              \
        }                                                                                                               \
        inline type const & Get(std::integral_constant<size_t, Index>) const                                            \
        {                                                                                                               \
            return member;                                                                                              \
        }                                                                                                               \
        inline auto AsTuple() const { return std::tuple_cat(Base::AsTuple(), std::make_tuple((type)member));  }         \
        inline auto AsRTuple() { return std::tuple_cat(Base::AsRTuple(), std::tie((type &)member));  }                  \
        inline auto AsRTuple() const { return std::tuple_cat(Base::AsRTuple(), std::tie((type const &)member));  }      \
        inline auto AsRCTuple() const { return AsRTuple();  }                                                           \
        inline char const * GetName(int index) const { return (index == Count - 1) ? #member : Base::GetName(index);  } \
        inline char const * GetDescription(int index) const                                                             \
        {                                                                                                               \
            static std::string desc { __VA_ARGS__ };                                                                    \
            return (index == Count - 1) ? desc.c_str() : Base::GetDescription(index);                                   \
        }                                                                                                               \
        inline int GetCount() const { return Count;  }                                                                  \
        inline std::string GetStringValue(int index) const                                                              \
        {                                                                                                               \
            if (index == Count - 1)                                                                                     \
            {                                                                                                           \
                std::stringstream ss;                                                                                   \
                ss << member;                                                                                           \
                return ss.str();                                                                                        \
            }                                                                                                           \
            else                                                                                                        \
            {                                                                                                           \
                return Base::GetStringValue(index);                                                                     \
            }                                                                                                           \
        }                                                                                                               \
        inline void Reset() { member = value; Base::Reset(); }                                                          \
    };

#define NAMEDTUPLE_ITEM(type, member, value, ...) \
    NAMEDTUPLE_ITEM_INTERNAL(__COUNTER__, type, member, value, __VA_ARGS__)

#define NAMEDTUPLE_END(name)                                        \
}                                                                   \
using name = name##Members::Member<__COUNTER__ - 1>;                \
template <size_t INDEX>                                             \
auto get(name & namedTuple)                                         \
{                                                                   \
    static_assert(INDEX < name::Count, "Index out of bounds");      \
    return namedTuple.Get(std::integral_constant<size_t, INDEX>()); \
}                                                                   \
template <size_t INDEX>                                             \
auto get(name const& namedTuple)                                    \
{                                                                   \
    static_assert(INDEX < name::Count, "Index out of bounds");      \
    return namedTuple.Get(std::integral_constant<size_t, INDEX>()); \
}

#endif


#include <string>

#if VERSION >= 1

NAMEDTUPLE_BEGIN(DataTest)
    NAMEDTUPLE_ITEM(std::string, Field1, "foo")
    NAMEDTUPLE_ITEM(std::string, Field2, "bar")
NAMEDTUPLE_END(DataTest)

#endif

#include <iostream>

#if VERSION >= 4

NAMEDTUPLE_BEGIN(DataTest2)
    NAMEDTUPLE_ITEM(std::string, Field1, "foo", "Some documented field")
    NAMEDTUPLE_ITEM(std::string, Field2, "bar")
    NAMEDTUPLE_ITEM(float, Field3, 3.14f, "Pi number")
    NAMEDTUPLE_ITEM(double, Field4, 0.3)
    NAMEDTUPLE_ITEM(int, Field5, 50)
NAMEDTUPLE_END(DataTest2)

struct DataTestStruct2
{
    std::string Field1;
    std::string Field2;
    float Field3;
    double Field4;
    int Field5;
};


template <typename NAMED_TUPLE>
void PrintStructure(NAMED_TUPLE const & namedTuple)
{
    for (int i = 0; i < namedTuple.GetCount(); ++i)
    {
        std::cout << namedTuple.GetName(i) << " = " << namedTuple.GetStringValue(i);
        if (namedTuple.GetDescription(i)[0] != '\0')
            std::cout << "\t# " << namedTuple.GetDescription(i);
        std::cout << std::endl;
    }
}

#endif

int main(int argc, char ** argv)
{
    std::tuple<std::string, std::string> classicTupleTest;

    std::get<0>(classicTupleTest) = "field1";
    std::get<1>(classicTupleTest) = "field2";

    std::cout << std::get<0>(classicTupleTest) << std::endl;
    std::cout << std::get<1>(classicTupleTest) << std::endl;

#if VERSION == 1
    system("cls || clear");

    DataTest test;
    test.Field2 = "toto";
    test.Field1 = "tata";

    std::cout << "Members count: " << DataTest::Count << std::endl;

    std::cout << test.Field2 << std::endl;
    std::cout << test.Field1 << std::endl;

#elif VERSION >= 2
    system("cls || clear");

    DataTest test;
    test.Field2 = "toto";
    test.Field1 = "tata";

    std::cout << "Members count: " << DataTest::Count << std::endl;

    std::cout << test.Field2 << std::endl;
    std::cout << test.Field1 << std::endl;

    std::cout << get<0>(test) << std::endl;
    std::cout << get<1>(test) << std::endl;
#endif

#if VERSION >= 3
    system("cls || clear");

    classicTupleTest = test.AsTuple();
    std::cout << get<0>(classicTupleTest) << std::endl;
    std::cout << get<1>(classicTupleTest) << std::endl;

    auto tiedTuple(test.AsRTuple());

    std::get<1>(tiedTuple) = "change toto";

    std::cout << test.Field2 << std::endl;
#endif

#if VERSION >= 4
    system("cls || clear");
    
    PrintStructure(test);

    DataTest2 test2;

    test2.Field1 = "test_field1";
    test2.Field2 = "test_field2";
    test2.Field4 = 40;

    std::cout << std::endl << "PrintStructure1 before Reset" << std::endl;
    PrintStructure(test2);

    std::cout << std::endl << "PrintStructure1 after Reset" << std::endl;
    test2.Reset();
    PrintStructure(test2);
#endif

    return EXIT_SUCCESS;
}
