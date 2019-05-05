// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234
//
// parser IO
#pragma once

#include "ParserBase.hpp"

#include <functional>
#include <list>
#include <iomanip>
#include <iostream>

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
}

template <typename INPUT, typename CHAR_TYPE>
class ParserIO
{
    using ErrorFunctionType = std::function<void(std::ostream &, std::string const &)>;

    Impl::InputAdapter<INPUT> input_;
    Impl::OutputAdapter<CHAR_TYPE> output_;
    std::list<ErrorFunctionType> errors_;
    std::list<ErrorFunctionType> lastRepeatErrors_;
public:
    inline ParserIO(INPUT input)
        : input_(input)
    {
    }

    inline auto const & OutputBuffer() const { return output_.Buffer(); }
    inline bool Ended() { return input_() == EOF; }
    inline Impl::InputAdapter<INPUT> & Input() { return input_; }
    inline Impl::OutputAdapter<CHAR_TYPE> & Output() { return output_; }
    inline auto const & Errors() const { return errors_; }
    inline auto & Errors() { return errors_; }
    inline auto & LastRepeatErrors() { return lastRepeatErrors_; }

public:
    template <bool REPEAT, bool ALT, typename RESULT_PTR>
    class SavedIOState : public SavedIOState<REPEAT, ALT, nullptr_t>
    {
        using Base = SavedIOState<REPEAT, ALT, nullptr_t>;
        using ResultType = std::remove_pointer_t<RESULT_PTR>;
    protected:
        RESULT_PTR result_;

    private:
        template <bool REPEAT2>
        static inline nullptr_t GetPreviousState(SubstringPos * result)
        {
            return nullptr;
        }

        template <bool REPEAT2, typename ELEM_TYPE>
        static inline size_t GetPreviousState(std::vector<ELEM_TYPE> * result)
        {
            return result->size();
        }

        template <bool REPEAT2, typename RESULT2_PTR, ENABLED_IF(REPEAT2 == false)>
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

        decltype(GetPreviousState<REPEAT>(result_)) previousState_;

        template <typename RESULT2_PTR>
        static inline void SetResult(RESULT2_PTR result, SubstringPos newResult)
        {
        }

        static inline void SetResult(SubstringPos * result, SubstringPos newResult)
        {
            *result = newResult;
        }
    public:
        inline SavedIOState(ParserIO & parent, RESULT_PTR result, char const * ruleName)
            : Base(parent, nullptr, ruleName), previousState_(GetPreviousState<REPEAT>(result)), result_(result)
        {
        }

        template <bool WHOLE>
        inline void Reset()
        {
            SetPreviousState(result_, previousState_);
            if (WHOLE)
                Base::template Reset<true>();
        }

        inline ~SavedIOState()
        {
            if (this->outputPos_ != -1)
            {
                Reset<false>();
            }
        }

        inline bool Success()
        {
            SetResult(result_, SubstringPos(this->outputPos_, this->parent_.Output().Pos()));
            return Base::Success();
        }

        inline void SetPossibleMatch() const
        {
            return Base::SetPossibleMatch();
        }

        inline bool HasPossibleMatch() const
        {
            return Base::HasPossibleMatch();
        }

    protected:
        inline RESULT_PTR Result()
        {
            return result_;
        }
    };
    
    template <bool REPEAT>
    class SavedIOState<REPEAT, false, nullptr_t>
    {
    protected:
        ParserIO & parent_;
        size_t inputPos_;
        size_t outputPos_;
        size_t errorsSize_;
        char const * ruleName_;
    public:
        inline SavedIOState(ParserIO & parent, nullptr_t, char const * ruleName)
            : parent_(parent), inputPos_(parent.Input().Pos()), outputPos_(parent.Output().Pos()),
            errorsSize_(parent.Errors().size()), ruleName_(ruleName)
        {
        }

        template <bool WHOLE>
        inline void Reset()
        {
            parent_.Input().SetPos(inputPos_);
            parent_.Output().SetPos(outputPos_);
        }

        inline ~SavedIOState()
        {
            if (inputPos_ != (size_t)-1 && outputPos_ != (size_t)-1)
            {
                std::list<ErrorFunctionType> childErrors;
                std::swap(parent_.Errors(), childErrors);
                parent_.Errors().push_back(
                    [ruleName = ruleName_, errors = std::move(childErrors), firstInputPos = inputPos_, lastInputPos = parent_.Input().Pos()]
                    (std::ostream & cerr, std::string const & indent)
                {
                    cerr << indent << "#" << firstInputPos;
                    if (firstInputPos < lastInputPos)
                        cerr << "-" << lastInputPos;
                    cerr << ": Error parsing rule [" << ruleName << "]" << std::endl;
                    for (auto const & error : errors)
                    {
                        error(cerr, indent + "  ");
                    }
                });
                Reset<false>();
            }
            else
            {
                parent_.Errors().resize(errorsSize_);
            }
        }

        inline bool Success()
        {
            inputPos_ = (size_t)-1;
            outputPos_ = (size_t)-1;
            return true;
        }

        inline void SetPossibleMatch() const
        {
        }

        inline bool HasPossibleMatch() const
        {
            return false;
        }

    protected:
        inline nullptr_t Result()
        {
            return nullptr;
        }
    };

    template <bool REPEAT, typename RESULT_PTR> 
    class SavedIOState<REPEAT, true, RESULT_PTR> : public SavedIOState<false, false, RESULT_PTR>
    {
        using Base = SavedIOState<false, false, RESULT_PTR>;
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
        inline void SetPossibleMatch()
        {
            size_t length(this->parent_.Input().Pos() - this->inputPos_);
            if (length > bestLength_)
            {
                bestLength_ = length;
                bestOutputPos_ = this->parent_.Output().Pos();
                bestInputPos_ = this->parent_.Input().Pos();
                SaveAlternative(PtrToBestAlternative(bestAlternative_), this->Result());
                // resets to the initial state to parse another alternative
                this->template Reset<true>();
            }
        }

        inline bool HasPossibleMatch() const
        {
            return bestLength_ > 0;
        }

        inline bool Success()
        {
            size_t length(this->parent_.Input().Pos() - this->inputPos_);
            if (length < bestLength_)
            {
                this->parent_.Output().SetPos(bestOutputPos_);
                this->parent_.Input().SetPos(bestInputPos_);
                SaveAlternative(this->Result(), PtrToBestAlternative(bestAlternative_));
            }
            return Base::Success();
        }
    public:
        inline SavedIOState(ParserIO & parent, RESULT_PTR result, char const * ruleName)
            : Base(parent, result, ruleName)
        {
        }
    };

    template <bool REPEAT, bool ALT, typename RESULT_PTR>
    inline auto Save(RESULT_PTR & result, char const * ruleName)
    {
        return SavedIOState<REPEAT, ALT, RESULT_PTR>(*this, result, ruleName);
    }
};

template <typename INPUT, typename CHAR_TYPE>
inline ParserIO<INPUT, CHAR_TYPE> Make_Parser(INPUT && input, CHAR_TYPE charType)
{
    return ParserIO<INPUT, CHAR_TYPE>(input);
}

template <typename CHAR_TYPE>
inline auto Make_ParserFromStream(std::basic_istream<CHAR_TYPE> & is)
{
    return Make_Parser([&] () mutable
    {
        return is.get();
    }, (CHAR_TYPE)0);
}

template <typename CHAR_TYPE>
inline auto Make_ParserFromString(std::basic_string<CHAR_TYPE> const & str)
{
    return Make_Parser([=, pos = (size_t)0] () mutable
    {
        if (pos < str.size())
        {
            return (int)str[pos++];
        }
        return EOF;
    }, (CHAR_TYPE)0);
}
