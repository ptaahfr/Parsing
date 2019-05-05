// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234
//
// grammar classes
#pragma once

#include "ParserBase.hpp"
#include <tuple>
#include <cctype>

template <MaxCharType... CODES>
class CharVal
{
public:
    static char const * Name() { return "CharVal"; }
};

template <MaxCharType CH1, MaxCharType CH2>
class CharRange
{
public:
    static char const * Name() { return "CharRange"; }
};

template <MaxCharType CH>
inline bool Match(int inputChar, CharVal<CH>)
{
    return inputChar == CH;
}

template <MaxCharType CH, MaxCharType ... OTHERS, ENABLED_IF(sizeof...(OTHERS) > 0)>
inline bool Match(int inputChar, CharVal<CH, OTHERS...>)
{
    return (inputChar == CH) || Match(inputChar, CharVal<OTHERS...>());
}

template <int CH1, int CH2>
inline bool Match(int inputChar, CharRange<CH1, CH2>)
{
    return inputChar >= CH1 && inputChar <= CH2;
}

template <typename BASE>
class DefaultDataT : public std::remove_reference_t<BASE>
{
};

template <>
class DefaultDataT<nullptr_t>
{
public:
    inline operator nullptr_t() const { return nullptr; }
};

#define PARSER_RULE_FORWARD(name) \
    class name { public: \
        static char const * Name() { return #name; } \
    }; \
    class IsConstant_##name IsConstant(name); \
    class IsRawVariable_##name IsRawVariable(name); \
    class DefaultData_##name DefaultData(name);

#define PARSER_RULE_PARTIAL(name, ...) \
    template <typename PARSER, typename TYPE> \
    inline bool Parse(PARSER & parser, TYPE result, char const *, name) { return Parse(parser, result, #name, __VA_ARGS__); } \
    template <typename PARSER, typename TYPE> \
    inline bool Parse(PARSER & parser, TYPE result, name) { return Parse(parser, result, #name, __VA_ARGS__); } \
    template <typename PARSER, typename TYPE> \
    inline bool ParseExact(PARSER & parser, TYPE result, name) \
    { \
        if (Parse(parser, result, #name, __VA_ARGS__)) \
        { \
            if (parser.Ended()) \
            { \
                return true;\
            } \
            parser.Errors().clear(); \
            std::swap(parser.Errors(), parser.LastRepeatErrors()); \
        } \
        return false; \
    } \
    class IsConstant_##name : public DefaultDataT<decltype(IsConstant(__VA_ARGS__))> {}; \
    class IsRawVariable_##name : public DefaultDataT<decltype(IsRawVariable(__VA_ARGS__))> {}; \
    class DefaultData_##name : public DefaultDataT<decltype(DefaultData(__VA_ARGS__))> {};

#define PARSER_RULE_PARTIAL_CDATA(name, data, ...) \
    PARSER_RULE_PARTIAL(name, __VA_ARGS__) \
    template <typename PARSER> \
    bool Parse(PARSER & parser, data * result) { return Parse(parser, result, name()); } \
    template <typename PARSER> \
    bool ParseExact(PARSER & parser, data * result) { return ParseExact(parser, result, name()); }

#define PARSER_RULE(name, ...) \
    PARSER_RULE_FORWARD(name) \
    PARSER_RULE_PARTIAL(name, __VA_ARGS__)

#define PARSER_RULE_CDATA(name, data, ...) \
    PARSER_RULE_FORWARD(name) \
    PARSER_RULE_PARTIAL_CDATA(name, data, __VA_ARGS__)

#define PARSER_RULE_DATA(name, ...) \
    PARSER_RULE_CDATA(name, name##Data, __VA_ARGS__)

#define INDEX_NONE INT_MAX          // Used to define a primitive that has no specific member
#define INDEX_THIS (INT_MAX-1)      // Use to define a primitive that parse current object instead of a member

template <size_t IDX>
class Idx {};

using SeqTypeSeq = std::integral_constant<size_t, 0>;
using SeqTypeAlt = std::integral_constant<size_t, 1>;
using SeqTypeUnion = std::integral_constant<size_t, 2>;

template <typename SEQ_TYPE, typename... PRIMITIVES>
class SequenceType
{
    std::tuple<PRIMITIVES...> primitives_;
public:
    inline SequenceType(PRIMITIVES... primitives)
        : primitives_(primitives...)
    {
    }

    static char const * Name() { return SEQ_TYPE::value == 0 ? "Sequence" : (SEQ_TYPE::value == 1 ? "Alternation" : "Union") ; }

    inline std::tuple<PRIMITIVES...> const & Primitives() const { return primitives_; }
    inline std::tuple<PRIMITIVES...> & Primitives() { return primitives_; }

};

// Sequence try to parse consecutive primitives in the current object's consecutive members or in the current object if there is only one variable primitive
template <typename... PRIMITIVES>
inline SequenceType<SeqTypeSeq, PRIMITIVES...> Sequence(PRIMITIVES... primitives)
{
    return SequenceType<SeqTypeSeq, PRIMITIVES...>(primitives...);
}

// Alternatives try to parse different primitive in the current object
template <typename... PRIMITIVES>
inline SequenceType<SeqTypeAlt, PRIMITIVES...> Alternatives(PRIMITIVES... primitives)
{
    return SequenceType<SeqTypeAlt, PRIMITIVES...>(primitives...);
}

// Union try to parse different primitive in the current object's consecutive members or in the current object if there is only one variable primitive
template <typename... PRIMITIVES>
inline SequenceType<SeqTypeUnion, PRIMITIVES...> Union(PRIMITIVES... primitives)
{
    return SequenceType<SeqTypeUnion, PRIMITIVES...>(primitives...);
}

template <size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
class RepeatType
{
    PRIMITIVE primitive_;
public:
    inline RepeatType(PRIMITIVE primitive)
        : primitive_(primitive)
    {
    }
    
    static char const * Name() { return "Repetition"; }

    inline PRIMITIVE const & Primitive() const { return primitive_; }
    inline PRIMITIVE & Primitive() { return primitive_; }
};

template <size_t MIN_COUNT = 0, size_t MAX_COUNT = SIZE_MAX, typename PRIMITIVE>
inline RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> Repeat(PRIMITIVE primitive)
{
    return RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE>(primitive);
}

template <size_t MIN_COUNT = 0, size_t MAX_COUNT = SIZE_MAX, typename PRIMITIVE, typename... OTHER_PRIMITIVES>
inline RepeatType<MIN_COUNT, MAX_COUNT, SequenceType<SeqTypeSeq, PRIMITIVE, OTHER_PRIMITIVES...> > Repeat(PRIMITIVE primitive, OTHER_PRIMITIVES... otherPrimitives)
{
    return Repeat<MIN_COUNT, MAX_COUNT>(Sequence(primitive, otherPrimitives...));
}

template <typename PRIMITIVE>
inline RepeatType<0, 1, PRIMITIVE> Optional(PRIMITIVE primitive)
{
    return RepeatType<0, 1, PRIMITIVE>(primitive);
}

template <typename PRIMITIVE, typename... OTHER_PRIMITIVES>
inline RepeatType<0, 1, SequenceType<SeqTypeSeq, PRIMITIVE, OTHER_PRIMITIVES...> > Optional(PRIMITIVE primitive, OTHER_PRIMITIVES... otherPrimitives)
{
    return Optional(Sequence(primitive, otherPrimitives...));
}

// Use head and tail when primitiveHead is the first elements of a list and primitiveTails contains the others elements of the list
template <typename PRIMITIVE, typename... OTHER_PRIMITIVES>
inline auto HeadTail(PRIMITIVE primitiveHead, OTHER_PRIMITIVES... primitiveTail)
-> decltype(Sequence(Idx<INDEX_THIS>(), Repeat<1, 1>(primitiveHead), Idx<INDEX_THIS>(), Repeat(primitiveTail...)))
{
    return Sequence(Idx<INDEX_THIS>(), Repeat<1, 1>(primitiveHead), Idx<INDEX_THIS>(), Repeat(primitiveTail...));
}


template <typename PRIMITIVE>
inline std::false_type IsConstant(PRIMITIVE const & primitive);

template <int CODE>
inline std::true_type IsConstant(CharVal<CODE> const & primitive);

template <int CH1, int CH2>
inline auto IsConstant(CharRange<CH1, CH2> const & primitive) -> decltype(std::bool_constant<(CH2 <= CH1)>());

template <typename SEQ_TYPE, typename PRIMITIVE>
inline auto IsConstant(SequenceType<SEQ_TYPE, PRIMITIVE> const & sequence) -> decltype(IsConstant(std::declval<PRIMITIVE>()));

template <typename PRIMITIVE, typename... OTHER_PRIMITIVES, ENABLED_IF(sizeof...(OTHER_PRIMITIVES) > 0)>
inline auto IsConstant(SequenceType<SeqTypeSeq, PRIMITIVE, OTHER_PRIMITIVES...> const & sequence)
-> std::bool_constant<CONSTANT(IsConstant(std::declval<PRIMITIVE>()))
                   && CONSTANT(IsConstant(std::declval<SequenceType<SeqTypeSeq, OTHER_PRIMITIVES...> >()))>;

template <size_t FIXED_COUNT, typename PRIMITIVE>
inline auto IsConstant(RepeatType<FIXED_COUNT, FIXED_COUNT, PRIMITIVE> const & sequence) -> decltype(IsConstant(std::declval<PRIMITIVE>()));

template <typename SEQ_TYPE, typename PRIMITIVE>
inline auto CountVariables(SequenceType<SEQ_TYPE, PRIMITIVE> const & sequence)
-> std::integral_constant<size_t, (CONSTANT(IsConstant(std::declval<PRIMITIVE>())) ? 0 : 1)>;

template <typename SEQ_TYPE, typename PRIMITIVE, typename... OTHER_PRIMITIVES, ENABLED_IF(sizeof...(OTHER_PRIMITIVES) > 0)>
inline auto CountVariables(SequenceType<SEQ_TYPE, PRIMITIVE, OTHER_PRIMITIVES...> const & sequence)
-> std::integral_constant<size_t, ((CONSTANT(IsConstant(std::declval<PRIMITIVE>())) ? 0 : 1)
                                  + CONSTANT(CountVariables(std::declval<SequenceType<SeqTypeSeq, OTHER_PRIMITIVES...> >())))>;

template <int CODE>
std::false_type IsRawVariable(CharVal<CODE> const & primitive);

template <int... CODES, ENABLED_IF(sizeof...(CODES) > 1)>
std::true_type IsRawVariable(CharVal<CODES...> const & primitive);

template <int CH1, int CH2>
auto IsRawVariable(CharRange<CH1, CH2> const & primitive) -> decltype(std::bool_constant<(CH2 > CH1)>());

template <typename SEQ_TYPE, typename PRIMITIVE>
auto IsRawVariable(SequenceType<SEQ_TYPE, PRIMITIVE> const & sequence) -> decltype(IsRawVariable(std::declval<PRIMITIVE>()));

template <size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
auto IsRawVariable(RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> const & sequence) -> decltype(IsRawVariable(std::declval<PRIMITIVE>()));

template <typename PRIMITIVE, typename... OTHER_PRIMITIVES, ENABLED_IF(sizeof...(OTHER_PRIMITIVES) > 0)>
auto IsRawVariable(SequenceType<SeqTypeSeq, PRIMITIVE, OTHER_PRIMITIVES...> const & sequence)
-> std::bool_constant<CONSTANT(IsRawVariable(std::declval<PRIMITIVE>()))
                   && CONSTANT(IsRawVariable(std::declval<SequenceType<SeqTypeSeq, OTHER_PRIMITIVES...> >()))>;

template <typename SEQ_TYPE, typename PRIMITIVE, typename... OTHER_PRIMITIVES,
    ENABLED_IF((SEQ_TYPE::value != SeqTypeSeq::value) && (sizeof...(OTHER_PRIMITIVES) > 0))>
auto IsRawVariable(SequenceType<SEQ_TYPE, PRIMITIVE, OTHER_PRIMITIVES...> const & sequence)
-> std::bool_constant<(CONSTANT(IsRawVariable(std::declval<PRIMITIVE>())) || CONSTANT(IsConstant(std::declval<PRIMITIVE>())))
                   && (CONSTANT(IsRawVariable(std::declval<SequenceType<SeqTypeSeq, OTHER_PRIMITIVES...> >()))
                        || CONSTANT(IsConstant(std::declval<SequenceType<SeqTypeSeq, OTHER_PRIMITIVES...> >())))>;

template <typename SEQ_TYPE, size_t INDEX, typename... OTHER_PRIMITIVES>
auto IsRawVariable(SequenceType<SEQ_TYPE, Idx<INDEX>, OTHER_PRIMITIVES...> const & sequence)
-> decltype(IsRawVariable(std::declval<SequenceType<SEQ_TYPE, OTHER_PRIMITIVES...> >()));

template <typename PRIMITIVE>
auto IsStructured(PRIMITIVE const & primitive)
-> std::bool_constant<
       (false == CONSTANT(IsRawVariable(std::declval<PRIMITIVE>()))
    && (false == CONSTANT(IsConstant(std::declval<PRIMITIVE>()))))>;

template <MaxCharType CODE, typename OSTREAM>
inline void CodesToString(OSTREAM & os)
{
    if (std::isprint(CODE))
        os << "\'" << (char)CODE << "\' ";
    os << "0x" << std::setfill('0') << std::setw(2) << CODE << std::hex;
}

template <MaxCharType CODE, MaxCharType... OTHER_CODES, typename OSTREAM, ENABLED_IF(sizeof...(OTHER_CODES) > 0)>
inline void CodesToString(OSTREAM & os)
{
    CodesToString<CODE>(os);
    os << " or ";
    CodesToString<OTHER_CODES...>(os);
}

template <typename PARSER, MaxCharType... CODES>
inline bool Parse(PARSER & parser, nullptr_t, char const * ruleName, CharVal<CODES...> const & what, bool escape = false)
{
    auto ch(parser.Input()());
    if (Match(ch, what))
    {
        parser.Output()(ch, escape);
        return true;
    }
    parser.Input().Back();
    parser.Errors().push_back([inputPos = parser.Input().Pos()](std::ostream & cerr, std::string const & indent)
    {
        cerr << indent << "#" << inputPos << ": Expected ";
        CodesToString<CODES...>(cerr);
        cerr << std::endl;
    });
    return false;
}

template <typename PARSER, MaxCharType CH1, MaxCharType CH2>
inline bool Parse(PARSER & parser, nullptr_t, char const * ruleName, CharRange<CH1, CH2> const & what, bool escape = false)
{
    auto ch(parser.Input()());
    if (Match(ch, what))
    {
        parser.Output()(ch, escape);
        return true;
    }
    parser.Input().Back();
    parser.Errors().push_back([inputPos = parser.Input().Pos()](std::ostream & cerr, std::string const & indent)
    {
        cerr << indent << "#" << inputPos << ": Expected range ";
        CodesToString<CH1>(cerr);
        cerr << "-";
        CodesToString<CH2>(cerr);
        cerr << std::endl;
    });
    return false;
}

namespace Impl
{
    template <size_t INDEX, typename TYPE_PTR, ENABLED_IF(INDEX != INDEX_THIS && INDEX != INDEX_NONE)>
    inline auto FieldNotNull(TYPE_PTR type) -> decltype(&std::get<INDEX>(*type))
    {
        if (type != nullptr)
        {
            return &std::get<INDEX>(*type);
        }
        return nullptr;
    }

    template <size_t INDEX, typename TYPE, ENABLED_IF(INDEX == INDEX_THIS)>
    inline TYPE * FieldNotNull(TYPE * type)
    {
        return type;
    }

    template <size_t INDEX, typename TYPE_PTR, ENABLED_IF(INDEX == INDEX_NONE)>
    inline nullptr_t FieldNotNull(TYPE_PTR type)
    {
        return nullptr;
    }

    template <size_t INDEX>
    inline nullptr_t FieldNotNull(SubstringPos * type)
    {
        return nullptr;
    }

    template <size_t INDEX>
    inline nullptr_t FieldNotNull(nullptr_t = nullptr)
    {
        return nullptr;
    }

    template <typename ELEM>
    inline void PushBackIfNotNull(std::vector<ELEM> * elems, ELEM const & elem)
    {
        if (elems != nullptr)
        {
            elems->push_back(elem);
        }
    }

    template <typename ELEM>
    inline void PushBackIfNotNull(SubstringPos *, ELEM const &)
    {
    }

    template <typename ELEM>
    inline void PushBackIfNotNull(nullptr_t, ELEM const &)
    {
    }

    template <typename ELEM>
    inline ELEM ElemTypeOrNull(std::vector<ELEM> const *);

    inline nullptr_t ElemTypeOrNull(nullptr_t);
    inline nullptr_t ElemTypeOrNull(SubstringPos *);

    template <typename ELEM>
    inline ELEM * PtrOrNull(ELEM & e)
    {
        return &e;
    }

    inline nullptr_t PtrOrNull(nullptr_t = nullptr)
    {
        return nullptr;
    }

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE,
        typename DEST_PTR, typename NEXT_ELEMENT, typename SEQ_TYPE, typename... PRIMITIVES, ENABLED_IF(OFFSET == sizeof...(PRIMITIVES) - 1)>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, NEXT_ELEMENT const & nextElement, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        enum { INDEX = CONSTANT(IsConstant(nextElement)) ? INDEX_NONE :
            (((SEQ_TYPE::value == SeqTypeAlt::value )
                || (CONSTANT(CountVariables(what)) <= 1)) ? INDEX_THIS : IMPLICIT_INDEX) };
        return Parse(parser, Impl::FieldNotNull<INDEX>(dest), nextElement.Name(), nextElement);
    }

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE,
        typename DEST_PTR, size_t INDEX, typename SEQ_TYPE, typename... PRIMITIVES, ENABLED_IF(OFFSET == sizeof...(PRIMITIVES) - 2)>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, Idx<INDEX> const &, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        return Parse(parser, Impl::FieldNotNull<INDEX>(dest), std::get<OFFSET + 1>(what.Primitives()).Name(), std::get<OFFSET + 1>(what.Primitives()));
    }

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE,
        typename DEST_PTR, typename NEXT_ELEMENT, typename SEQ_TYPE, typename... PRIMITIVES, ENABLED_IF(OFFSET < sizeof...(PRIMITIVES) - 1)>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, NEXT_ELEMENT const & nextElement, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what);

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE,
        typename DEST_PTR, size_t INDEX, typename SEQ_TYPE, typename... PRIMITIVES, ENABLED_IF(OFFSET < sizeof...(PRIMITIVES) - 2)>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, Idx<INDEX> const &, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        auto const & nextElement(std::get<OFFSET + 1>(what.Primitives()));

        bool result = Parse(parser, Impl::FieldNotNull<INDEX>(dest), nextElement.Name(), nextElement);
        if (result)
            ioState.SetPossibleMatch();

        if ((SEQ_TYPE::value != SeqTypeSeq::value) || result)
            return ParseItem<OFFSET + 2, IMPLICIT_INDEX>(parser, ioState, dest, std::get<OFFSET + 2>(what.Primitives()), what) || ioState.HasPossibleMatch();
        else
            return ioState.HasPossibleMatch();
    }

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE,
        typename DEST_PTR, typename NEXT_ELEMENT, typename SEQ_TYPE, typename... PRIMITIVES, ENABLED_IF_DEF(OFFSET < sizeof...(PRIMITIVES) - 1)>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, NEXT_ELEMENT const & nextElement, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        enum { INDEX = CONSTANT(IsConstant(nextElement)) ? INDEX_NONE :
            (((SEQ_TYPE::value == SeqTypeAlt::value )
                || (CONSTANT(CountVariables(what)) <= 1)) ? INDEX_THIS : IMPLICIT_INDEX) };

        enum { NEXT_IMPLICIT_INDEX = (INDEX == IMPLICIT_INDEX ? IMPLICIT_INDEX + 1 : IMPLICIT_INDEX) };

        bool result = Parse(parser, Impl::FieldNotNull<INDEX>(dest), nextElement.Name(), nextElement);
        if (result)
            ioState.SetPossibleMatch();

        if ((SEQ_TYPE::value != SeqTypeSeq::value) || result)
            return ParseItem<OFFSET + 1, NEXT_IMPLICIT_INDEX>(parser, ioState, dest, std::get<OFFSET + 1>(what.Primitives()), what) || ioState.HasPossibleMatch();
        else
            return ioState.HasPossibleMatch();
    }
}

template <typename PARSER, typename DEST_PTR, typename SEQ_TYPE, typename... PRIMITIVES>
inline bool Parse(PARSER & parser, DEST_PTR dest, char const * ruleName, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
{
    auto ioState(parser.template Save<false, SEQ_TYPE::value != SeqTypeSeq::value>(dest, ruleName));
    if (Impl::ParseItem<0, 0>(parser, ioState, dest, std::get<0>(what.Primitives()), what))
    {
        return ioState.Success();
    }
    return false;
}

template <typename PARSER, typename ELEMS_PTR, size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
inline bool Parse(PARSER & parser, ELEMS_PTR elems, char const * ruleName, RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> const & what)
{
    size_t count = 0;

    auto ioState(parser.template Save<true, false>(elems, ruleName));

    for (; count < MAX_COUNT; ++count)
    {
        decltype(Impl::ElemTypeOrNull(elems)) elem = {};
        if (Parse(parser, Impl::PtrOrNull(elem), what.Primitive().Name(), what.Primitive()))
        {
            if (false == IsEmpty(elem))
                Impl::PushBackIfNotNull(elems, std::move(elem));
        }
        else
        {
            break;
        }
    }

    if (count >= MIN_COUNT)
    {
        parser.LastRepeatErrors().clear();
        std::swap(parser.LastRepeatErrors(), parser.Errors());
        return ioState.Success();
    }
    return false;
}

template <typename PARSER, typename ELEMS_PTR, typename PRIMITIVE>
inline bool Parse(PARSER & parser, ELEMS_PTR elems, char const * ruleName, RepeatType<0, 1, PRIMITIVE> const & what)
{
    return Parse(parser, elems, what.Primitive().Name(), what.Primitive()) || true;
}

template <typename PRIMITIVE, ENABLED_IF(CONSTANT(IsConstant(std::declval<PRIMITIVE>())))>
nullptr_t DefaultData(PRIMITIVE const & primitive);

template <typename PRIMITIVE, ENABLED_IF(CONSTANT(IsRawVariable(std::declval<PRIMITIVE>())))>
SubstringPos DefaultData(PRIMITIVE const & primitive);

template <size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
auto DefaultData(RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> const & repetition)
-> std::vector<decltype(DefaultData(repetition.Primitive()))>;

template <typename SEQ_TYPE, typename PRIMITIVE>
auto DefaultData(SequenceType<SEQ_TYPE, PRIMITIVE> const & sequence)
-> decltype(DefaultData(std::get<0>(sequence)));

template <typename SEQ_TYPE, typename PRIMITIVE, typename... OTHER_PRIMITIVES,
    ENABLED_IF((0 < sizeof...(OTHER_PRIMITIVES))
        && CONSTANT(IsStructured(std::declval<SequenceType<SEQ_TYPE, PRIMITIVE, OTHER_PRIMITIVES...> >())))>
auto DefaultData(SequenceType<SEQ_TYPE, PRIMITIVE, OTHER_PRIMITIVES...> const & sequence)
-> decltype(std::tuple_cat(
    std::make_tuple(DefaultData(std::declval<PRIMITIVE>())),
    DefaultData(std::declval<SequenceType<SEQ_TYPE, OTHER_PRIMITIVES...> >())));
