// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234
//
// parser IO and base grammar classes
#pragma once

#include <vector>
#include <cassert>
#include <tuple>
#include <type_traits>
#include <string>
#include <algorithm>

#define INDEX_NONE INT_MAX          // Used to define a primitive that has no specific member
#define INDEX_THIS (INT_MAX-1)      // Use to define a primitive that parse current object instead of a member

using MaxCharType = int;

using SubstringPos = std::pair<size_t, size_t>;

namespace Impl
{
    template <typename CHAR_TYPE>
    class OutputAdapter
    {
        std::vector<CHAR_TYPE> buffer_;
        size_t bufferPos_;
    public:
        inline OutputAdapter()
            : bufferPos_(0)
        {
        }

        inline std::vector<CHAR_TYPE> const & Buffer() const
        {
            return buffer_;
        }

        inline void operator()(CHAR_TYPE ch, bool isEscapeChar = false)
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

        inline size_t Pos() const
        {
            return bufferPos_;
        }

        inline void SetPos(size_t pos)
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
        inline InputAdapter(INPUT input)
            : input_(input), bufferPos_(0)
        {
        }

        inline MaxCharType operator()()
        {
            if (bufferPos_ >= buffer_.size())
            {
                MaxCharType ch = (MaxCharType)input_();
                buffer_.push_back(ch);
            }
            assert(bufferPos_ < buffer_.size());

            return buffer_[bufferPos_++];
        }

        inline void Back()
        {
            assert(bufferPos_ > 0);
            bufferPos_--;
        }

        inline bool GetIf(InputResult value)
        {
            if (value == (*this)())
            {
                return true;
            }
            Back();
            return false;
        }

        inline size_t Pos() const
        {
            return bufferPos_;
        }

        inline void SetPos(size_t pos)
        {
            bufferPos_ = pos;
        }
    };

    template <size_t INDEX, typename TYPE_PTR, std::enable_if_t<(INDEX != INDEX_THIS && INDEX != INDEX_NONE), void *> = nullptr>
    inline auto FieldNotNull(TYPE_PTR type) -> decltype(&std::get<INDEX>(*type))
    {
        if (type != nullptr)
        {
            return &std::get<INDEX>(*type);
        }
        return nullptr;
    }

    template <size_t INDEX, typename TYPE, std::enable_if_t<(INDEX == INDEX_THIS), void *> = nullptr>
    inline TYPE * FieldNotNull(TYPE * type)
    {
        return type;
    }

    template <size_t INDEX, typename TYPE_PTR, std::enable_if_t<(INDEX == INDEX_NONE), void *> = nullptr>
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
    inline void ResizeIfNotNull(std::vector<ELEM> * elems, size_t newSize)
    {
        if (elems != nullptr)
        {
            elems->resize(newSize);
        }
    }

    inline void ResizeIfNotNull(SubstringPos *, size_t newSize)
    {
    }

    inline void ResizeIfNotNull(nullptr_t, size_t newSize)
    {
    }

    template <typename ELEM>
    inline size_t GetSizeIfNotNull(std::vector<ELEM> const * elems)
    {
        if (elems != nullptr)
        {
            return elems->size();
        }
        return 0;
    }

    inline size_t GetSizeIfNotNull(SubstringPos *)
    {
        return 0;
    }

    inline size_t GetSizeIfNotNull(nullptr_t)
    {
        return 0;
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

}

template <MaxCharType... CODES>
class CharVal
{
};

template <MaxCharType CH1, MaxCharType CH2>
class CharRange
{
};

template <MaxCharType CH>
inline bool Match(int inputChar, CharVal<CH>)
{
    return inputChar == CH;
}

template <MaxCharType CH, MaxCharType ... OTHERS, std::enable_if_t<(sizeof...(OTHERS) > 0), void *> = nullptr>
inline bool Match(int inputChar, CharVal<CH, OTHERS...>)
{
    return (inputChar == CH) || Match(inputChar, CharVal<OTHERS...>());
}

template <int CH1, int CH2>
inline bool Match(int inputChar, CharRange<CH1, CH2>)
{
    return inputChar >= CH1 && inputChar <= CH2;
}


#define PARSER_RULE_FORWARD(name) \
    class name {};

#define PARSER_RULE_PARTIAL(name, ...) \
    template <typename PARSER, typename TYPE> \
    inline bool Parse(PARSER & parser, TYPE result, name) { return Parse(parser, result, __VA_ARGS__); } \
    auto IsConstant(name) -> decltype(IsConstant(__VA_ARGS__));

#define PARSER_RULE_PARTIAL_CDATA(name, data, ...) \
    PARSER_RULE_PARTIAL(name, __VA_ARGS__) \
    template <typename PARSER> \
    bool Parse(PARSER & parser, data * result) { return Parse(parser, result, __VA_ARGS__); }

#define PARSER_RULE(name, ...) \
    class name {}; \
    PARSER_RULE_PARTIAL(name, __VA_ARGS__)

#define PARSER_RULE_CDATA(name, data, ...) \
    PARSER_RULE_FORWARD(name) \
    PARSER_RULE_PARTIAL_CDATA(name, data, __VA_ARGS__)

#define PARSER_RULE_DATA(name, ...) \
    PARSER_RULE_CDATA(name, name##Data, __VA_ARGS__)

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

template <typename PRIMITIVE, typename... OTHER_PRIMITIVES, std::enable_if_t<(sizeof...(OTHER_PRIMITIVES) > 0), void *> = nullptr>
inline auto IsConstant(SequenceType<SeqTypeSeq, PRIMITIVE, OTHER_PRIMITIVES...> const & sequence)
-> std::bool_constant<std::remove_reference_t<decltype(IsConstant(std::declval<PRIMITIVE>()))>::value
                   && std::remove_reference_t<decltype(IsConstant(std::declval<SequenceType<SeqTypeSeq, OTHER_PRIMITIVES...> >()))>::value>;

template <size_t FIXED_COUNT, typename PRIMITIVE>
inline auto IsConstant(RepeatType<FIXED_COUNT, FIXED_COUNT, PRIMITIVE> const & sequence) -> decltype(IsConstant(std::declval<PRIMITIVE>()));

template <typename SEQ_TYPE, typename PRIMITIVE>
inline auto CountVariable(SequenceType<SEQ_TYPE, PRIMITIVE> const & sequence)
-> std::integral_constant<size_t, (std::remove_reference_t<decltype(IsConstant(std::declval<PRIMITIVE>()))>::value ? 0 : 1)>;

template <typename SEQ_TYPE, typename PRIMITIVE, typename... OTHER_PRIMITIVES, std::enable_if_t<(sizeof...(OTHER_PRIMITIVES) > 0), void *> = nullptr>
inline auto CountVariable(SequenceType<SEQ_TYPE, PRIMITIVE, OTHER_PRIMITIVES...> const & sequence)
-> std::integral_constant<size_t, ((std::remove_reference_t<decltype(IsConstant(std::declval<PRIMITIVE>()))>::value ? 0 : 1)
                                  + std::remove_reference_t<decltype(CountVariable(std::declval<SequenceType<SeqTypeSeq, OTHER_PRIMITIVES...> >()))>::value)>;

template <typename INPUT, typename CHAR_TYPE>
class ParserIO
{
private:
    Impl::InputAdapter<INPUT> input_;
    Impl::OutputAdapter<CHAR_TYPE> output_;
public:
    inline ParserIO(INPUT input)
        : input_(input)
    {
    }

    inline auto const & OutputBuffer() const { return output_.Buffer(); }
    inline bool Ended() { return input_() == EOF; }
    inline Impl::InputAdapter<INPUT> & Input() { return input_; }
    inline Impl::OutputAdapter<CHAR_TYPE> & Output() { return output_; }

public:
    template <bool ALT, typename RESULT_PTR>
    class SavedIOState : public SavedIOState<ALT, nullptr_t>
    {
        using Base = SavedIOState<ALT, nullptr_t>;
        using ResultType = std::remove_pointer_t<RESULT_PTR>;
    protected:
        RESULT_PTR result_;

    private:
        template <typename ELEM_TYPE>
        static inline size_t GetPreviousState(std::vector<ELEM_TYPE> * result)
        {
            return result->size();
        }

        template <typename RESULT2_PTR>
        static inline nullptr_t GetPreviousState(RESULT2_PTR result)
        {
            return nullptr;
        }


        template <typename ELEM_TYPE>
        static inline void SetPreviousState(std::vector<ELEM_TYPE> * result, size_t previousState)
        {
            result->resize(previousState);
        }

        template <typename RESULT>
        static inline void SetPreviousState(RESULT * result, nullptr_t)
        {
            *result = {};
        }

        decltype(GetPreviousState(result_)) previousState_;

        template <typename RESULT2_PTR>
        static inline void SetResult(RESULT2_PTR result, SubstringPos newResult)
        {
        }

        static inline void SetResult(SubstringPos * result, SubstringPos newResult)
        {
            *result = newResult;
        }
    public:
        inline SavedIOState(Impl::InputAdapter<INPUT> & input, Impl::OutputAdapter<CHAR_TYPE> & output, RESULT_PTR result)
            : Base(input, output, nullptr), previousState_(GetPreviousState(result)), result_(result)
        {
        }

        inline ~SavedIOState()
        {
            if (this->outputPos_ != -1)
            {
                SetPreviousState(result_, previousState_);
            }
        }

        inline bool Success()
        {
            SetResult(result_, SubstringPos(this->outputPos_, this->output_.Pos()));
            return Base::Success();
        }

        inline void Maybe() const
        {
            return Base::Maybe();
        }

        inline bool Possible() const
        {
            return Base::Possible();
        }

    protected:
        inline RESULT_PTR Result()
        {
            return result_;
        }
    };
    
    template <>
    class SavedIOState<false, nullptr_t>
    {
    protected:
        Impl::InputAdapter<INPUT> & input_;
        Impl::OutputAdapter<CHAR_TYPE> & output_;
        size_t inputPos_;
        size_t outputPos_;
    public:
        inline SavedIOState(Impl::InputAdapter<INPUT> & input, Impl::OutputAdapter<CHAR_TYPE> & output, nullptr_t)
            : input_(input), output_(output), inputPos_(input.Pos()), outputPos_(output.Pos())
        {
        }

        inline ~SavedIOState()
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

        inline bool Success()
        {
            inputPos_ = (size_t)-1;
            outputPos_ = (size_t)-1;
            return true;
        }

        inline void Maybe() const
        {
        }

        inline bool Possible() const
        {
            return false;
        }

    protected:
        inline nullptr_t Result()
        {
            return nullptr;
        }
    };

    template <typename RESULT_PTR> 
    class SavedIOState<true, RESULT_PTR> : public SavedIOState<false, RESULT_PTR>
    {
        using Base = SavedIOState<false, RESULT_PTR>;
        size_t bestLength_ = 0;
        size_t bestOutputPos_ = 0;
        size_t bestInputPos_ = 0;

        static inline nullptr_t SaveAlternative(nullptr_t, nullptr_t)
        {
            return nullptr;
        }

        template <typename RESULT>
        static inline RESULT & SaveAlternative(RESULT * destination, RESULT * source)
        {
            return *destination = *source;
        }

        std::remove_reference_t<decltype(SaveAlternative(std::declval<RESULT_PTR>(), std::declval<RESULT_PTR>()))> bestAlternative_;
        
        template <typename RESULT>
        static inline RESULT * PtrToBestAlternative(RESULT & r)
        {
            return &r;
        }

        static inline nullptr_t PtrToBestAlternative(nullptr_t)
        {
            return nullptr;
        }

    public:
        inline void Maybe()
        {
            size_t length(this->input_.Pos() - this->inputPos_);
            if (length > bestLength_)
            {
                bestLength_ = length;
                bestOutputPos_ = this->output_.Pos();
                bestInputPos_ = this->input_.Pos();
                SaveAlternative(PtrToBestAlternative(bestAlternative_), this->Result());
                // resets to the initial state to parse another alternative
                this->~SavedIOState();
            }
        }

        inline bool Possible() const
        {
            return bestLength_ > 0;
        }

        inline bool Success()
        {
            size_t length(this->input_.Pos() - this->inputPos_);
            if (length < bestLength_)
            {
                this->output_.SetPos(bestOutputPos_);
                this->input_.SetPos(bestInputPos_);
                SaveAlternative(this->Result(), PtrToBestAlternative(bestAlternative_));
            }
            return Base::Success();
        }
    public:
        inline SavedIOState(Impl::InputAdapter<INPUT> & input, Impl::OutputAdapter<CHAR_TYPE> & output, RESULT_PTR result)
            : Base(input, output, result)
        {
        }
    };

    template <bool ALT, typename RESULT_PTR>
    inline SavedIOState<ALT, RESULT_PTR> Save(RESULT_PTR & result)
    {
        return SavedIOState<ALT, RESULT_PTR>(input_, output_, result);
    }
};

template <typename INPUT, typename CHAR_TYPE>
inline ParserIO<INPUT, CHAR_TYPE> Make_Parser(INPUT && input, CHAR_TYPE charType)
{
    return ParserIO<INPUT, CHAR_TYPE>(input);
}

template <typename CHAR_TYPE>
inline auto Make_ParserFromString(std::basic_string<CHAR_TYPE> const & str)
{
    return Make_Parser([str, pos = (size_t)0] () mutable
    {
        if (pos < str.size())
            return (int)str[pos++];
        return EOF;
    }, (CHAR_TYPE)0);
}

template <typename PARSER, MaxCharType... CODES>
inline bool Parse(PARSER & parser, nullptr_t, CharVal<CODES...> const & what, bool escape = false)
{
    auto ch(parser.Input()());
    if (Match(ch, what))
    {
        parser.Output()(ch, escape);
        return true;
    }
    parser.Input().Back();
    return false;
}


template <typename PARSER, MaxCharType CH1, MaxCharType CH2>
inline bool Parse(PARSER & parser, nullptr_t, CharRange<CH1, CH2> const & what, bool escape = false)
{
    auto ch(parser.Input()());
    if (Match(ch, what))
    {
        parser.Output()(ch, escape);
        return true;
    }
    parser.Input().Back();
    return false;
}

namespace Impl
{
    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE, typename DEST_PTR, typename NEXT_ELEMENT, typename SEQ_TYPE, typename... PRIMITIVES, std::enable_if_t<(OFFSET == sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, NEXT_ELEMENT const & nextElement, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        enum { INDEX = std::remove_reference_t<decltype(IsConstant(nextElement))>::value ? INDEX_NONE :
            (((SEQ_TYPE::value == SeqTypeAlt::value )
                || (std::remove_reference_t<decltype(CountVariable(what))>::value <= 1)) ? INDEX_THIS : IMPLICIT_INDEX) };
        return Parse(parser, Impl::FieldNotNull<INDEX>(dest), nextElement);
    }

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE, typename DEST_PTR, size_t INDEX, typename SEQ_TYPE, typename... PRIMITIVES, std::enable_if_t<(OFFSET == sizeof...(PRIMITIVES) - 2), void *> = nullptr>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, Idx<INDEX> const &, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        return Parse(parser, Impl::FieldNotNull<INDEX>(dest), std::get<OFFSET + 1>(what.Primitives()));
    }

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE, typename DEST_PTR, typename NEXT_ELEMENT, typename SEQ_TYPE, typename... PRIMITIVES, std::enable_if_t<(OFFSET < sizeof...(PRIMITIVES) - 1), void *> = nullptr>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, NEXT_ELEMENT const & nextElement, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what);

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE, typename DEST_PTR, size_t INDEX, typename SEQ_TYPE, typename... PRIMITIVES, std::enable_if_t<(OFFSET < sizeof...(PRIMITIVES) - 2), void *> = nullptr>
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, Idx<INDEX> const &, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        auto const & nextElement(std::get<OFFSET + 1>(what.Primitives()));

        bool result = Parse(parser, Impl::FieldNotNull<INDEX>(dest), nextElement);
        ioState.Maybe();

        if ((SEQ_TYPE::value != SeqTypeSeq::value) || result)
            return ParseItem<OFFSET + 2, IMPLICIT_INDEX>(parser, ioState, dest, std::get<OFFSET + 2>(what.Primitives()), what) || ioState.Possible();
        else
            return ioState.Possible();
    }

    template <size_t OFFSET, size_t IMPLICIT_INDEX, typename PARSER, typename IO_STATE, typename DEST_PTR, typename NEXT_ELEMENT, typename SEQ_TYPE, typename... PRIMITIVES, std::enable_if_t<(OFFSET < sizeof...(PRIMITIVES) - 1), void *> >
    inline bool ParseItem(PARSER & parser, IO_STATE & ioState, DEST_PTR dest, NEXT_ELEMENT const & nextElement, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
    {
        enum { INDEX = std::remove_reference_t<decltype(IsConstant(nextElement))>::value ? INDEX_NONE :
            (((SEQ_TYPE::value == SeqTypeAlt::value )
                || (std::remove_reference_t<decltype(CountVariable(what))>::value <= 1)) ? INDEX_THIS : IMPLICIT_INDEX) };

        enum { NEXT_IMPLICIT_INDEX = (INDEX == IMPLICIT_INDEX ? IMPLICIT_INDEX + 1 : IMPLICIT_INDEX) };

        bool result = Parse(parser, Impl::FieldNotNull<INDEX>(dest), nextElement);
        ioState.Maybe();
        
        if ((SEQ_TYPE::value != SeqTypeSeq::value) || result)
            return ParseItem<OFFSET + 1, NEXT_IMPLICIT_INDEX>(parser, ioState, dest, std::get<OFFSET + 1>(what.Primitives()), what) || ioState.Possible();
        else
            return ioState.Possible();
    }
}

template <typename PARSER, typename DEST_PTR, typename SEQ_TYPE, typename... PRIMITIVES>
inline bool Parse(PARSER & parser, DEST_PTR dest, SequenceType<SEQ_TYPE, PRIMITIVES...> const & what)
{
    auto ioState(parser.Save<SEQ_TYPE::value != SeqTypeSeq::value>(dest));
    if (Impl::ParseItem<0, 0>(parser, ioState, dest, std::get<0>(what.Primitives()), what))
    {
        return ioState.Success();
    }
    return false;
}

template <typename PARSER, typename ELEMS_PTR, size_t MIN_COUNT, size_t MAX_COUNT, typename PRIMITIVE>
inline bool Parse(PARSER & parser, ELEMS_PTR elems, RepeatType<MIN_COUNT, MAX_COUNT, PRIMITIVE> const & what)
{
    size_t count = 0;

    auto ioState(parser.Save<false>(elems));

    for (; count < MAX_COUNT; ++count)
    {
        decltype(Impl::ElemTypeOrNull(elems)) elem = {};
        if (Parse(parser, Impl::PtrOrNull(elem), what.Primitive()))
        {
            Impl::PushBackIfNotNull(elems, elem);
        }
        else
        {
            break;
        }
    }

    if (count < MIN_COUNT)
    {
        return false;
    }

    return ioState.Success();
}

template <typename PARSER, typename ELEMS_PTR, typename PRIMITIVE>
inline bool Parse(PARSER & parser, ELEMS_PTR elems, RepeatType<0, 1, PRIMITIVE> const & what)
{
    return Parse(parser, elems, what.Primitive()) || true;
}

template <typename CHAR_TYPE>
inline std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, SubstringPos subString)
{
    subString.first = std::min(subString.first, buffer.size());
    subString.second = std::max(subString.first, std::min(subString.second, buffer.size()));
    return std::basic_string<CHAR_TYPE>(buffer.data() + subString.first, buffer.data() + subString.second);
}

inline bool IsEmpty(SubstringPos sub)
{
    return sub.second <= sub.first;
}

template <typename ELEM>
inline bool IsEmpty(std::vector<ELEM> arr);

template <size_t OFFSET, typename... ARGS>
inline bool IsEmpty(std::tuple<ARGS...> tuple);

template <typename... ARGS>
inline bool IsEmpty(std::tuple<ARGS...> tuple)
{
    return IsEmpty<0>(tuple);
}

template <size_t OFFSET, typename... ARGS>
inline bool IsEmpty(std::tuple<ARGS...> tuple)
{
    enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(ARGS) - 1) };
    if (false == IsEmpty(std::get<OFFSET>(tuple)))
        return false;
    if (NEXT_OFFSET > OFFSET)
        return IsEmpty<NEXT_OFFSET>(tuple);
    return true;
}

template <typename ELEM>
inline bool IsEmpty(std::vector<ELEM> arr)
{
    for (auto const & elem : arr)
    {
        if (false == IsEmpty(elem))
            return false;
    }
    return true;
}

