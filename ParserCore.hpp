// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234
//
// some rules from the ABNF RFC core
#pragma once

#include "ParserBase.hpp"

// Core rules as defined in the Appendix B https://tools.ietf.org/html/rfc5234#appendix-B

namespace RFC5234Core
{
    // ALPHA          =  %x41-5A / %x61-7A   ; A-Z / a-z
    PARSER_RULE(ALPHA, Alternatives(CharRange<0x41, 0x5A>(), CharRange<0x61, 0x7A>()));
    // BIT            =  "0" / "1"
    PARSER_RULE(BIT, CharVal<'0', '1'>());
    // CHAR           =  %x01-7F
    PARSER_RULE(CHAR, CharRange<0x01, 0x7F>());
    // LF             =  %x0A
    PARSER_RULE(LF, CharVal<0x0A>());
    // CR             =  %x0D
    PARSER_RULE(CR, CharVal<0x0D>());
    // CRLF           =  CR LF
    PARSER_RULE(CRLF, Sequence(CR(), LF()));
    // CTL            =  %x00-1F / %x7F
    PARSER_RULE(CTL, Alternatives(CharRange<0x00, 0x1F>(), CharVal<0x7F>()))
    // DIGIT          =  %x30-39
    PARSER_RULE(DIGIT, CharRange<0x30, 0x39>());
    // DQUOTE         =  %x22
    PARSER_RULE(DQUOTE, CharVal<0x22>());
    // HEXDIG         =  DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
    PARSER_RULE(HEXDIG, Alternatives(DIGIT(), CharRange<'A', 'F'>()))
    // HTAB           =  %x09
    PARSER_RULE(HTAB, CharVal<0x09>());
    // OCTET          =  %x00-FF
    PARSER_RULE(OCTET, CharRange<0x00, 0xFF>());
    // SP             =  %x20
    PARSER_RULE(SP, CharVal<0x20>());
    // VCHAR          =  %x21-7E
    PARSER_RULE(VCHAR, CharRange<0x21, 0x7E>());
    // WSP            =  SP / HTAB
    PARSER_RULE(WSP, Alternatives(SP(), HTAB()));
    // LWSP           =  *(WSP / CRLF WSP)
    PARSER_RULE(LWSP, Repeat(Alternatives(WSP(), Sequence(CRLF(), WSP()))));
}
