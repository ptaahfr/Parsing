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
    PARSER_RULE_CHARPRED(ALPHA, [] (auto ch) { return ch >= 'A' && ch <= 'Z' || ch >= 'a' && ch <= 'z'; });
    // BIT            =  "0" / "1"
    PARSER_RULE_CHARPRED(BIT, [] (auto ch) { return ch == '0' || ch == '1'; });
    // CHAR           =  %x01-7F
    PARSER_RULE_CHARPRED(CHAR, [] (auto ch) { return ch >= 1 && ch <= 0x7f; });
    // LF             =  %x0A
    PARSER_RULE(LF, CharExact('\n'));
    // CR             =  %x0D
    PARSER_RULE(CR, CharExact('\r'));
    // CRLF           =  CR LF
    PARSER_RULE(CRLF, Sequence(CR(), LF()));
    // CTL            =  %x00-1F / %x7F
    PARSER_RULE_CHARPRED(CTL, [] (auto ch) { return ch >= 0x00 && ch <= 0x1f || ch == 0x7f; });
    // DIGIT          =  %x30-39
    PARSER_RULE_CHARPRED(DIGIT, [] (auto ch) { return ch >= '0' && ch <= '9'; });
    // HEXDIG         =  DIGIT / "A" / "B" / "C" / "D" / "E" / "F"
    PARSER_RULE_CHARPRED(HEXDIG, [] (auto ch) { return ch >= '0' && ch <= '9' || ch >= 'A' && ch <= 'F'; });
    // HTAB           =  %x09
    PARSER_RULE(HTAB, CharExact('\t'));
    // OCTET          =  %x00-FF
    PARSER_RULE_CHARPRED(OCTET, [] (auto ch) { return ch >= 0 && ch <= 255; });
    // SP             =  %x20
    PARSER_RULE(SP, CharExact(' '));
    // VCHAR          =  %x21-7E
    PARSER_RULE_CHARPRED(VCHAR, [] (auto ch) { return ch >= 0x21 && ch <= 0x7e; });
    // WSP            =  SP / HTAB
    PARSER_RULE(WSP, Alternatives(SP(), HTAB()));
    // LWSP           =  *(WSP / CRLF WSP)
    PARSER_RULE(LWSP, Repeat(Alternatives(WSP(), Sequence(CRLF(), WSP()))));
}
