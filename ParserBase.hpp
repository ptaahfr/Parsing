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

size_t GetSizeIfNotNull(nullptr_t)
{
    return 0;
}

template <typename ELEM>
ELEM ElemTypeOrNull(std::vector<ELEM> const *);

nullptr_t ElemTypeOrNull(nullptr_t = nullptr);

template <size_t S1, size_t S2, typename PTR, std::enable_if_t<S1 == S2, void *> = nullptr>
PTR NonNullIfEqual(PTR ptr)
{
    return ptr;
}

template <size_t S1, size_t S2, typename PTR, std::enable_if_t<S1 != S2, void *> = nullptr>
nullptr_t NonNullIfEqual(PTR ptr)
{
    return nullptr;
}

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

#define DECLARE_PARSER_PRIMITIVE_T1(name, t1) \
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

#define DECLARE_PARSER_PRIMITIVE(name) \
    class name##Type { } name;

DECLARE_PARSER_PRIMITIVE_T1(CharPred, PREDICATE)
DECLARE_PARSER_PRIMITIVE_T1(CharExact, CHAR_TYPE)

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
class ParserBase
{
protected:
    InputAdapter<INPUT> input_;
    OutputAdapter<CHAR_TYPE> output_;
public:
    ParserBase(INPUT input)
        : input_(input)
    {
    }

    auto const & OutputBuffer() const { return output_.Buffer(); }
    bool Ended() { return input_() == EOF; }
protected:
    class SavedIOState
    {
        InputAdapter<INPUT> & input_;
        OutputAdapter<CHAR_TYPE> & output_;
        size_t inputPos_;
        size_t outputPos_;
    public:
        SavedIOState(InputAdapter<INPUT> & input, OutputAdapter<CHAR_TYPE> & output)
            : input_(input), output_(output), inputPos_(input.Pos()), outputPos_(output.Pos())
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
            inputPos_ = (size_t)-1;
            outputPos_ = (size_t)-1;
            return true;
        }
    };

    SavedIOState Save()
    {
        return SavedIOState(input_, output_);
    }

    template <typename ELEMS_PTR, typename PARSER>
    bool ParseRepeat(ELEMS_PTR elems, PARSER parser, size_t minCount = 0, size_t maxCount = SIZE_MAX)
    {
        size_t count = 0;
        size_t prevSize = GetSizeIfNotNull(elems);

        auto ioState(this->Save());

        for (; count < maxCount; ++count)
        {
            decltype(ElemTypeOrNull(elems)) elem = {};
            if (parser(PtrOrNull(elem)))
            {
                PushBackIfNotNull(elems, elem);
            }
            else
            {
                break;
            }
        }

        if (count < minCount)
        {
            ResizeIfNotNull(elems, prevSize);
            return false;
        }

        return ioState.Success();
    }

    template <typename ELEM_PTR, typename PARSER>
    bool ParseOptional(ELEM_PTR elem, PARSER parser)
    {
        return parser(elem) || true;
    }
    
    template <typename PREDICATE>
    bool Parse(nullptr_t, CharPredType<PREDICATE> const & what)
    {
        auto ch(this->input_());
        if (std::get<0>(what)(ch))
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    template <typename CHAR_TYPE>
    bool Parse(nullptr_t, CharExactType<CHAR_TYPE> const & what, bool escape = false)
    {
        auto ch(std::get<0>(what));
        if (input_.GetIf(ch))
        {
            this->output_(ch, escape);
            return true;
        }
        return false;
    }

private:
    template <size_t OFFSET, typename THAT, typename DEST_PTR, typename... PRIMITIVES>
    static bool ParseItem(THAT & that, DEST_PTR dest, SequenceType<std::false_type, PRIMITIVES...> const & what)
    {
        enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(PRIMITIVES) - 1) };
        if (that.Parse(FIELD_NOTNULL(dest, OFFSET), std::get<OFFSET>(what)))
        {
            if (OFFSET < NEXT_OFFSET)
                return ParseItem<NEXT_OFFSET>(that, dest, what);
            else
                return true;
        }
        return false;
    }

    template <size_t OFFSET, typename THAT, typename DEST_PTR, typename... PRIMITIVES>
    static bool ParseItem(THAT & that, DEST_PTR dest, SequenceType<std::true_type, PRIMITIVES...> const & what)
    {
        enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(PRIMITIVES) - 1) };

        if (that.Parse(FIELD_NOTNULL(dest, OFFSET), std::get<OFFSET>(what)))
            return true;

        if (OFFSET < NEXT_OFFSET)
            return ParseItem<NEXT_OFFSET>(that, dest, what);

        return false;
    }

protected:
    // This one is static with THAT so we have the "full class" with all the primitives overloads
    template <typename THAT, typename DEST_PTR, typename IS_ALT, typename... PRIMITIVES>
    static bool Parse(THAT & that, DEST_PTR dest, SequenceType<IS_ALT, PRIMITIVES...> const & what)
    {
        auto ioState(that.Save());
        if (ParseItem<0>(that, dest, what))
        {
            return ioState.Success();
        }
        CLEAR_NOTNULL(dest);
        return false;
    }

private:
    template <size_t OFFSET, typename THAT, typename DEST_PTR, size_t INDEX, typename IS_ALT, typename INDEX_SEQUENCE, typename... PRIMITIVES, std::enable_if_t<(OFFSET == sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    static bool ParseItem(THAT & that, DEST_PTR dest, std::index_sequence<INDEX> indices, IndexedSequenceType<IS_ALT, INDEX_SEQUENCE, PRIMITIVES...> const & what)
    {
        return that.Parse(FieldNotNull<INDEX>(dest), std::get<OFFSET>(what));
    }

    template <size_t OFFSET, typename THAT, typename DEST_PTR, size_t INDEX, size_t... OTHER_INDICES, typename INDEX_SEQUENCE, typename... PRIMITIVES, std::enable_if_t<(OFFSET < sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    static bool ParseItem(THAT & that, DEST_PTR dest, std::index_sequence<INDEX, OTHER_INDICES...> indices, IndexedSequenceType<std::true_type, INDEX_SEQUENCE, PRIMITIVES...> const & what)
    {
        if (that.Parse(FieldNotNull<INDEX>(dest), std::get<OFFSET>(what)))
        {
            return true;
        }

        return ParseItem<OFFSET + 1>(that, dest, std::index_sequence<OTHER_INDICES...>(), what);
    }

    template <size_t OFFSET, typename THAT, typename DEST_PTR, size_t INDEX, size_t... OTHER_INDICES, typename INDEX_SEQUENCE, typename... PRIMITIVES, std::enable_if_t<(OFFSET < sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    static bool ParseItem(THAT & that, DEST_PTR dest, std::index_sequence<INDEX, OTHER_INDICES...> indices, IndexedSequenceType<std::false_type, INDEX_SEQUENCE, PRIMITIVES...> const & what)
    {
        if (that.Parse(FieldNotNull<INDEX>(dest), std::get<OFFSET>(what)))
        {
            return ParseItem<OFFSET + 1>(that, dest, std::index_sequence<OTHER_INDICES...>(), what);
        }
        return false;
    }

protected:
    // This one is static with THAT so we have the "full class" with all the primitives overloads
    template <typename PARSER, typename DEST_PTR, typename IS_ALT, typename INDEX_SEQUENCE, typename... PRIMITIVES>
    static bool Parse(PARSER & parser, DEST_PTR dest, IndexedSequenceType<IS_ALT, INDEX_SEQUENCE, PRIMITIVES...> const & what)
    {
        auto ioState(parser.Save());
        if (ParseItem<0>(parser, dest, INDEX_SEQUENCE(), what))
        {
            return ioState.Success();
        }
        CLEAR_NOTNULL(dest);
        return false;
    }

    template <typename PARSER, typename ELEMS_PTR, size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
    static bool Parse(PARSER & parser, ELEMS_PTR elems, RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> const & what)
    {
        size_t count = 0;
        size_t prevSize = GetSizeIfNotNull(elems);

        auto ioState(parser.Save());

        for (; count < MAX_COUNT; ++count)
        {
            decltype(ElemTypeOrNull(elems)) elem = {};
            if (parser.Parse(PtrOrNull(elem), what.Primitive()))
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
    static bool Parse(PARSER & parser, ELEMS_PTR elems, HeadType<PRIMITIVE> const & what)
    {
        size_t prevSize = GetSizeIfNotNull(elems);

        auto ioState(parser.Save());

        decltype(ElemTypeOrNull(elems)) elem = {};
        if (parser.Parse(PtrOrNull(elem), what.Primitive()))
        {
            PushBackIfNotNull(elems, elem);
            return ioState.Success();
        }
        
        ResizeIfNotNull(elems, prevSize);

        return false;
    }

    template <typename PARSER, typename ELEMS_PTR, typename PRIMITIVE>
    static bool Parse(PARSER & parser, ELEMS_PTR elems, OptionalType<PRIMITIVE> const & what)
    {
        return parser.Parse(elems, what.Primitive()) || true;
    }
};

using SubstringPos = std::pair<size_t, size_t>;

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

