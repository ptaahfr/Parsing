#pragma once

#include "ParserBase.hpp"

// Core rules as defined in the Appendix B https://tools.ietf.org/html/rfc5234#appendix-B

namespace RFC5234Core
{
    // VCHAR          =  %x21-7E
    PARSER_RULE(VCHAR, CharPred([](auto ch) { return ch >= 0x10 && ch <= 0x7e; }));
    // WSP            =  SP / HTAB
    PARSER_RULE(WSP, CharPred([](auto ch) { return ch == ' ' || ch == '\t'; }));
    // CRLF           =  CR LF
    PARSER_RULE(CRLF, Sequence(CharExact('\r'), CharExact('\n')));
}
