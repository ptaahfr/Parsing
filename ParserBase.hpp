#pragma once

#include <vector>
#include <cassert>
#include <tuple>
#include <type_traits>
#include <string>
#include <algorithm>

#define INDEX_NONE SIZE_MAX
#define INDEX_THIS (SIZE_MAX-1)

template <typename CHAR_TYPE>
class OutputAdapter
{
    std::vector<CHAR_TYPE> buffer_;
    size_t bufferPos_;
public:
    OutputAdapter()
        : bufferPos_(0)
    {
    }

    std::vector<CHAR_TYPE> const & Buffer() const
    {
        return buffer_;
    }

    void operator()(CHAR_TYPE ch, bool isEscapeChar = false)
    {
        if (isEscapeChar)
            return;

        if (bufferPos_ >= buffer_.size())
        {
            buffer_.push_back(ch);
        }
        else
        {
            assert(bufferPos_ < buffer_.size());
            buffer_[bufferPos_] = ch;
        }
        bufferPos_++;
    }

    size_t Pos() const
    {
        return bufferPos_;
    }

    void SetPos(size_t pos)
    {
        bufferPos_ = pos;
    }
};

template <typename INPUT>
class InputAdapter
{
    INPUT input_;
    using InputResult = decltype(std::declval<INPUT>()());
    std::vector<InputResult> buffer_;
    size_t bufferPos_;
public:
    InputAdapter(INPUT input)
        : input_(input), bufferPos_(0)
    {
    }

    int operator()()
    {
        if (bufferPos_ >= buffer_.size())
        {
            int ch = input_();
            buffer_.push_back(ch);
        }
        assert(bufferPos_ < buffer_.size());

        return buffer_[bufferPos_++];
    }

    void Back()
    {
        assert(bufferPos_ > 0);
        bufferPos_--;
    }

    bool GetIf(InputResult value)
    {
        if (value == (*this)())
        {
            return true;
        }
        Back();
        return false;
    }

    size_t Pos() const
    {
        return bufferPos_;
    }

    void SetPos(size_t pos)
    {
        bufferPos_ = pos;
    }
};

using SubstringPos = std::pair<size_t, size_t>;

template <size_t INDEX, typename TYPE_PTR, std::enable_if_t<(INDEX != INDEX_THIS && INDEX != INDEX_NONE), void *> = nullptr>
auto FieldNotNull(TYPE_PTR type) -> decltype(&std::get<INDEX>(*type))
{
    if (type != nullptr)
    {
        return &std::get<INDEX>(*type);
    }
    return nullptr;
}

template <size_t INDEX, typename TYPE, std::enable_if_t<(INDEX == INDEX_THIS), void *> = nullptr>
TYPE * FieldNotNull(TYPE * type)
{
    return type;
}

template <size_t INDEX, typename TYPE_PTR, std::enable_if_t<(INDEX == INDEX_NONE), void *> = nullptr>
nullptr_t FieldNotNull(TYPE_PTR type)
{
    return nullptr;
}

template <size_t INDEX>
nullptr_t FieldNotNull(SubstringPos * type)
{
    return nullptr;
}

template <size_t INDEX>
nullptr_t FieldNotNull(nullptr_t = nullptr)
{
    return nullptr;
}

template <typename TYPE>
void ClearNotNull(TYPE * t)
{
    if (t != nullptr)
    {
        *t = {};
    }
}

void ClearNotNull(nullptr_t = nullptr)
{
}

template <typename ELEM>
void PushBackIfNotNull(std::vector<ELEM> * elems, ELEM const & elem)
{
    if (elems != nullptr)
    {
        elems->push_back(elem);
    }
}

template <typename ELEM>
void PushBackIfNotNull(SubstringPos *, ELEM const &)
{
}

template <typename ELEM>
void PushBackIfNotNull(nullptr_t, ELEM const &)
{
}

template <typename ELEM>
void ResizeIfNotNull(std::vector<ELEM> * elems, size_t newSize)
{
    if (elems != nullptr)
    {
        elems->resize(newSize);
    }
}

void ResizeIfNotNull(SubstringPos *, size_t newSize)
{
}

void ResizeIfNotNull(nullptr_t, size_t newSize)
{
}

template <typename ELEM>
size_t GetSizeIfNotNull(std::vector<ELEM> const * elems)
{
    if (elems != nullptr)
    {
        return elems->size();
    }
    return 0;
}

size_t GetSizeIfNotNull(SubstringPos *)
{
    return 0;
}

size_t GetSizeIfNotNull(nullptr_t)
{
    return 0;
}

template <typename ELEM>
ELEM ElemTypeOrNull(std::vector<ELEM> const *);

nullptr_t ElemTypeOrNull(nullptr_t);
nullptr_t ElemTypeOrNull(SubstringPos *);

template <typename ELEM>
ELEM * PtrOrNull(ELEM & e)
{
    return &e;
}

nullptr_t PtrOrNull(nullptr_t = nullptr)
{
    return nullptr;
}

#define FIELD_NOTNULL(m, x) FieldNotNull<x>(m)
#define CLEAR_NOTNULL(m) ClearNotNull(m)

#define THIS_PARSE(X) [&] (auto elem) { return this->X(elem); }
#define THIS_PARSE_ARGS(X, ...) [&] (auto elem) { return this->X(elem, __VA_ARGS__); }
#define PARSE(...) [&] (auto elem) { return this->Parse(elem, __VA_ARGS__); }

#define PARSER_CUSTOM_PRIMITIVE_T1(name, t1) \
    template <typename t1> \
    class name##Type : public std::tuple<t1> \
    { \
    public: \
        name##Type(t1 arg) : std::tuple<t1>(arg) {} \
    }; \
    template <typename t1> \
    name##Type<t1> name(t1 arg) \
    { \
        return name##Type<t1>(arg); \
    };

#define PARSER_CUSTOM_PRIMITIVE(name) \
    class name##Type { } name;

PARSER_CUSTOM_PRIMITIVE_T1(CharPred, PREDICATE)
PARSER_CUSTOM_PRIMITIVE_T1(CharExact, CHAR_TYPE)

#define PARSER_RULE_FORWARD(name) \
    class name;

#define PARSER_RULE(name, ...) \
    class name {}; \
    template <typename PARSER, typename TYPE> \
    bool Parse(PARSER & parser, TYPE result, name) { return Parse(parser, result, __VA_ARGS__); }

#define PARSER_RULE_DATA(name, ...) \
    PARSER_RULE(name, __VA_ARGS__) \
    template <typename PARSER> \
    bool Parse(PARSER & parser, name##Data * result) { return Parse(parser, result, __VA_ARGS__); }

template <typename IS_ALT, typename... PRIMITIVES>
class SequenceType : public std::tuple<PRIMITIVES...>
{
public:
    SequenceType(PRIMITIVES... primitives)
        : std::tuple<PRIMITIVES...>(primitives...)
    {
    }
};

template <typename... PRIMITIVES>
SequenceType<std::false_type, PRIMITIVES...> Sequence(PRIMITIVES... primitives)
{
    return SequenceType<std::false_type, PRIMITIVES...>(primitives...);
}

template <typename... PRIMITIVES>
SequenceType<std::true_type, PRIMITIVES...> Alternatives(PRIMITIVES... primitives)
{
    return SequenceType<std::true_type, PRIMITIVES...>(primitives...);
}

template <typename IS_ALT, typename INDEX_SEQUENCE, typename... PRIMITIVES>
class IndexedSequenceType : public std::tuple<PRIMITIVES...>
{
public:
    IndexedSequenceType(PRIMITIVES... primitives)
        : std::tuple<PRIMITIVES...>(primitives...)
    {
    }
};

template <size_t... INDICES, typename... PRIMITIVES>
IndexedSequenceType<std::false_type, std::index_sequence<INDICES...>, PRIMITIVES...> IndexedSequence(std::index_sequence<INDICES...>, PRIMITIVES... primitives)
{
    return IndexedSequenceType<std::false_type, std::index_sequence<INDICES...>, PRIMITIVES...>(primitives...);
}

template <size_t... INDICES, typename... PRIMITIVES>
IndexedSequenceType<std::true_type, std::index_sequence<INDICES...>, PRIMITIVES...> IndexedAlternatives(std::index_sequence<INDICES...>, PRIMITIVES... primitives)
{
    return IndexedSequenceType<std::true_type, std::index_sequence<INDICES...>, PRIMITIVES...>(primitives...);
}

template <size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
class RepeatType
{
    PRIMITIVE primitive_;
public:
    RepeatType(PRIMITIVE primitive)
        : primitive_(primitive)
    {
    }

    PRIMITIVE const & Primitive() const { return primitive_; }
    PRIMITIVE & Primitive() { return primitive_; }
};

template <size_t MIN_COUNT = 0, size_t MAX_COUNT = SIZE_MAX, typename PRIMITIVE>
RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> Repeat(PRIMITIVE primitive)
{
    return RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE>(primitive);
}

template <size_t MIN_COUNT = 0, size_t MAX_COUNT = SIZE_MAX, typename PRIMITIVE, typename... OTHER_PRIMITIVES>
RepeatType<MIN_COUNT, MAX_COUNT, SequenceType<std::false_type, PRIMITIVE, OTHER_PRIMITIVES...> > Repeat(PRIMITIVE primitive, OTHER_PRIMITIVES... otherPrimitives)
{
    return Repeat<MIN_COUNT, MAX_COUNT>(Sequence(primitive, otherPrimitives...));
}

template <typename PRIMITIVE>
class OptionalType
{
    PRIMITIVE primitive_;
public:
    OptionalType(PRIMITIVE primitive)
        : primitive_(primitive)
    {
    }

    PRIMITIVE const & Primitive() const { return primitive_; }
    PRIMITIVE & Primitive() { return primitive_; }
};

template <typename PRIMITIVE>
OptionalType<PRIMITIVE> Optional(PRIMITIVE primitive)
{
    return OptionalType<PRIMITIVE>(primitive);
}

template <typename PRIMITIVE, typename... OTHER_PRIMITIVES>
OptionalType<SequenceType<std::false_type, PRIMITIVE, OTHER_PRIMITIVES...> > Optional(PRIMITIVE primitive, OTHER_PRIMITIVES... otherPrimitives)
{
    return Optional(Sequence(primitive, otherPrimitives...));
}

template <typename PRIMITIVE>
class HeadType
{
    PRIMITIVE primitive_;
public:
    HeadType(PRIMITIVE primitive)
        : primitive_(primitive)
    {
    }

    PRIMITIVE const & Primitive() const { return primitive_; }
    PRIMITIVE & Primitive() { return primitive_; }
};

template <typename PRIMITIVE>
HeadType<PRIMITIVE> Head(PRIMITIVE primitive)
{
    return HeadType<PRIMITIVE>(primitive);
}


template <typename INPUT, typename CHAR_TYPE>
class ParserIO
{
private:
    InputAdapter<INPUT> input_;
    OutputAdapter<CHAR_TYPE> output_;
public:
    ParserIO(INPUT input)
        : input_(input)
    {
    }

    auto const & OutputBuffer() const { return output_.Buffer(); }
    bool Ended() { return input_() == EOF; }
    InputAdapter<INPUT> & Input() { return input_; }
    OutputAdapter<CHAR_TYPE> & Output() { return output_; }

public:
    template <typename RESULT_PTR>
    class SavedIOState
    {
        InputAdapter<INPUT> & input_;
        OutputAdapter<CHAR_TYPE> & output_;
        size_t inputPos_;
        size_t outputPos_;
        RESULT_PTR result_;

        template <typename RESULT2_PTR>
        static void SetResult(RESULT2_PTR result, SubstringPos newResult)
        {
        }

        static void SetResult(SubstringPos * result, SubstringPos newResult)
        {
            *result = newResult;
        }
    public:
        SavedIOState(InputAdapter<INPUT> & input, OutputAdapter<CHAR_TYPE> & output, RESULT_PTR result)
            : input_(input), output_(output), inputPos_(input.Pos()), outputPos_(output.Pos()), result_(result)
        {
        }

        ~SavedIOState()
        {
            if (inputPos_ != (size_t)-1)
            {
                input_.SetPos(inputPos_);
            }
            if (outputPos_ != (size_t)-1)
            {
                output_.SetPos(outputPos_);
            }
        }

        bool Success()
        {
            SetResult(result_, SubstringPos(outputPos_, output_.Pos()));
            inputPos_ = (size_t)-1;
            outputPos_ = (size_t)-1;
            return true;
        }
    };

    template <typename RESULT_PTR>
    SavedIOState<RESULT_PTR> Save(RESULT_PTR result)
    {
        return SavedIOState<RESULT_PTR>(input_, output_, result);
    }
};

template <typename INPUT, typename CHAR_TYPE>
ParserIO<INPUT, CHAR_TYPE> Make_Parser(INPUT && input, CHAR_TYPE charType)
{
    return ParserIO<INPUT, CHAR_TYPE>(input);
}

template <typename PARSER, typename PREDICATE>
bool Parse(PARSER & parser, nullptr_t, CharPredType<PREDICATE> const & what)
{
    auto ch(parser.Input()());
    if (std::get<0>(what)(ch))
    {
        parser.Output()(ch);
        return true;
    }
    parser.Input().Back();
    return false;
}

template <typename PARSER, typename CHAR_TYPE>
bool Parse(PARSER & parser, nullptr_t, CharExactType<CHAR_TYPE> const & what, bool escape = false)
{
    auto ch(std::get<0>(what));
    if (parser.Input().GetIf(ch))
    {
        parser.Output()(ch, escape);
        return true;
    }
    return false;
}

namespace Impl
{
    template <size_t OFFSET, typename PARSER, typename DEST_PTR, typename... PRIMITIVES>
    bool ParseItem(PARSER & parser, DEST_PTR dest, SequenceType<std::false_type, PRIMITIVES...> const & what)
    {
        enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(PRIMITIVES) - 1) };
        if (Parse(parser, FIELD_NOTNULL(dest, OFFSET), std::get<OFFSET>(what)))
        {
            if (OFFSET < NEXT_OFFSET)
                return ParseItem<NEXT_OFFSET>(parser, dest, what);
            else
                return true;
        }
        return false;
    }

    template <size_t OFFSET, typename PARSER, typename DEST_PTR, typename... PRIMITIVES>
    bool ParseItem(PARSER & parser, DEST_PTR dest, SequenceType<std::true_type, PRIMITIVES...> const & what)
    {
        enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(PRIMITIVES) - 1) };

        if (Parse(parser, FIELD_NOTNULL(dest, INDEX_THIS), std::get<OFFSET>(what)))
            return true;

        if (OFFSET < NEXT_OFFSET)
            return ParseItem<NEXT_OFFSET>(parser, dest, what);

        return false;
    }
}

template <typename PARSER, typename DEST_PTR, typename IS_ALT, typename... PRIMITIVES>
bool Parse(PARSER & parser, DEST_PTR dest, SequenceType<IS_ALT, PRIMITIVES...> const & what)
{
    auto ioState(parser.Save(dest));
    if (Impl::ParseItem<0>(parser, dest, what))
    {
        return ioState.Success();
    }
    CLEAR_NOTNULL(dest);
    return false;
}

namespace Impl
{
    template <size_t OFFSET, typename PARSER, typename DEST_PTR, size_t INDEX, typename IS_ALT, typename INDEX_SEQUENCE, typename... PRIMITIVES, std::enable_if_t<(OFFSET == sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    bool ParseItem(PARSER & parser, DEST_PTR dest, std::index_sequence<INDEX> indices, IndexedSequenceType<IS_ALT, INDEX_SEQUENCE, PRIMITIVES...> const & what)
    {
        return Parse(parser, FieldNotNull<INDEX>(dest), std::get<OFFSET>(what));
    }

    template <size_t OFFSET, typename PARSER, typename DEST_PTR, size_t INDEX, size_t... OTHER_INDICES, typename INDEX_SEQUENCE, typename... PRIMITIVES, std::enable_if_t<(OFFSET < sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    bool ParseItem(PARSER & parser, DEST_PTR dest, std::index_sequence<INDEX, OTHER_INDICES...> indices, IndexedSequenceType<std::true_type, INDEX_SEQUENCE, PRIMITIVES...> const & what)
    {
        if (Parse(parser, FieldNotNull<INDEX>(dest), std::get<OFFSET>(what)))
        {
            return true;
        }

        return ParseItem<OFFSET + 1>(parser, dest, std::index_sequence<OTHER_INDICES...>(), what);
    }

    template <size_t OFFSET, typename PARSER, typename DEST_PTR, size_t INDEX, size_t... OTHER_INDICES, typename INDEX_SEQUENCE, typename... PRIMITIVES, std::enable_if_t<(OFFSET < sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    bool ParseItem(PARSER & parser, DEST_PTR dest, std::index_sequence<INDEX, OTHER_INDICES...> indices, IndexedSequenceType<std::false_type, INDEX_SEQUENCE, PRIMITIVES...> const & what)
    {
        if (Parse(parser, FieldNotNull<INDEX>(dest), std::get<OFFSET>(what)))
        {
            return ParseItem<OFFSET + 1>(parser, dest, std::index_sequence<OTHER_INDICES...>(), what);
        }
        return false;
    }
}

template <typename PARSER, typename DEST_PTR, typename IS_ALT, typename INDEX_SEQUENCE, typename... PRIMITIVES>
bool Parse(PARSER & parser, DEST_PTR dest, IndexedSequenceType<IS_ALT, INDEX_SEQUENCE, PRIMITIVES...> const & what)
{
    auto ioState(parser.Save(dest));
    if (Impl::ParseItem<0>(parser, dest, INDEX_SEQUENCE(), what))
    {
        return ioState.Success();
    }
    CLEAR_NOTNULL(dest);
    return false;
}

template <typename PARSER, typename ELEMS_PTR, size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
bool Parse(PARSER & parser, ELEMS_PTR elems, RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> const & what)
{
    size_t count = 0;
    size_t prevSize = GetSizeIfNotNull(elems);

    auto ioState(parser.Save(elems));

    for (; count < MAX_COUNT; ++count)
    {
        decltype(ElemTypeOrNull(elems)) elem = {};
        if (Parse(parser, PtrOrNull(elem), what.Primitive()))
        {
            PushBackIfNotNull(elems, elem);
        }
        else
        {
            break;
        }
    }

    if (count < MIN_COUNT)
    {
        ResizeIfNotNull(elems, prevSize);
        return false;
    }

    return ioState.Success();
}

template <typename PARSER, typename ELEMS_PTR, typename PRIMITIVE>
bool Parse(PARSER & parser, ELEMS_PTR elems, HeadType<PRIMITIVE> const & what)
{
    size_t prevSize = GetSizeIfNotNull(elems);

    auto ioState(parser.Save(elems));

    decltype(ElemTypeOrNull(elems)) elem = {};
    if (Parse(parser, PtrOrNull(elem), what.Primitive()))
    {
        PushBackIfNotNull(elems, elem);
        return ioState.Success();
    }
        
    ResizeIfNotNull(elems, prevSize);

    return false;
}

template <typename PARSER, typename ELEMS_PTR, typename PRIMITIVE>
bool Parse(PARSER & parser, ELEMS_PTR elems, OptionalType<PRIMITIVE> const & what)
{
    return Parse(parser, elems, what.Primitive()) || true;
}

template <typename CHAR_TYPE>
std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, SubstringPos subString)
{
    subString.first = std::min(subString.first, buffer.size());
    subString.second = std::max(subString.first, std::min(subString.second, buffer.size()));
    return std::basic_string<CHAR_TYPE>(buffer.data() + subString.first, buffer.data() + subString.second);
}

bool IsEmpty(SubstringPos sub)
{
    return sub.second <= sub.first;
}

template <typename ELEM>
bool IsEmpty(std::vector<ELEM> arr);

template <size_t OFFSET, typename... ARGS>
bool IsEmpty(std::tuple<ARGS...> tuple);

template <typename... ARGS>
bool IsEmpty(std::tuple<ARGS...> tuple)
{
    return IsEmpty<0>(tuple);
}

template <size_t OFFSET, typename... ARGS>
bool IsEmpty(std::tuple<ARGS...> tuple)
{
    enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(ARGS) - 1) };
    if (false == IsEmpty(std::get<OFFSET>(tuple)))
        return false;
    if (NEXT_OFFSET > OFFSET)
        return IsEmpty<NEXT_OFFSET>(tuple);
    return true;
}


template <typename ELEM>
bool IsEmpty(std::vector<ELEM> arr)
{
    for (auto const & elem : arr)
    {
        if (false == IsEmpty(elem))
            return false;
    }
    return true;
}

