#pragma once

#include "ParserBase.hpp"

// Core rules as defined in the Appendix B https://tools.ietf.org/html/rfc5234#appendix-B

// VCHAR          =  %x21-7E
static auto const VCHAR = CharPred([](auto ch) { return ch >= 0x10 && ch <= 0x7e; });
// WSP            =  SP / HTAB
static auto const WSP = CharPred([](auto ch) { return ch == ' ' || ch == '\t'; });
// CRLF           =  CR LF
PARSER_CUSTOM_PRIMITIVE(CRLF)

template <typename INPUT, typename CHAR_TYPE = char>
class ParserCore : public ParserBase<INPUT, CHAR_TYPE>
{
    using Base = ParserBase<INPUT, CHAR_TYPE>;
public:
    ParserCore(INPUT input)
        : Base(input)
    {
    }

    template <typename... ARGS>
    auto Parse(ARGS &&... args) -> decltype(Base::Parse(args...))
    {
        return Base::Parse(args...);
    }

    //template <typename... ARGS>
    //auto Parse(ARGS &&... args) -> decltype(Base::Parse(*this, args...))
    //{
    //    return Base::Parse(*this, args...);
    //}

    bool Parse(nullptr_t, CRLFType)
    {
        // CRLF        =  %d13.10
        if (this->input_.GetIf('\r'))
        {
            if (this->input_.GetIf('\n'))
            {
                this->output_('\r');
                this->output_('\n');
                return true;
            }
        }

        return false;
    }
};
