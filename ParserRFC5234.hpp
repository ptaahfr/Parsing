// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// ABNF rules
#pragma once

#include "ParserBase.hpp"
#include "ParserCore.hpp"


namespace RFC5234ABNF
{
    using namespace RFC5234Core;

    // rulename       =  ALPHA *(ALPHA / DIGIT / "-")
    PARSER_RULE(rulename, Sequence(ALPHA(), Repeat(Alternatives(ALPHA(), DIGIT(), CharExact('-')))));

    // comment        =  ";" *(WSP / VCHAR) CRLF
    PARSER_RULE(comment, Sequence(CharExact(';'), Repeat(Alternatives(WSP(), VCHAR())), CRLF()))

    // c-nl           =  comment / CRLF
    //                 ; comment or newline
    PARSER_RULE(c_nl, Alternatives(comment(), CRLF()));

    // c-wsp          =  WSP / (c-nl WSP)
    PARSER_RULE(c_wsp, Alternatives(WSP(), Sequence(c_nl(), WSP())));

    // defined-as     =  *c-wsp ("=" / "=/") *c-wsp
    //                ; basic rules definition and
    //                ;  incremental alternatives
    PARSER_RULE(defined_as, Sequence(Repeat(c_wsp()), Alternatives(CharExact('='), Sequence(CharExact('='), CharExact('/'))), Repeat(c_wsp)));

    // repeat         =  1*DIGIT / (*DIGIT "*" *DIGIT)
    PARSER_RULE(repeat, Alternatives(Repeat<1>(DIGIT()), Sequence(Repeat(DIGIT()), CharExact('*'), Repeat(DIGIT()))));

    PARSER_RULE_FORWARD(alternation);

    // group          =  "(" *c-wsp alternation *c-wsp ")"
    PARSER_RULE(group, Sequence(CharExact('('), Repeat(c_wsp()), alternation(), Repeat(c_wsp()), CharExact(')')));

    // option         =  "[" *c-wsp alternation *c-wsp "]"
    PARSER_RULE(option, Sequence(CharExact('['), Repeat(c_wsp()), alternation(), Repeat(c_wsp()), CharExact(']')));
    
    // prose-val      =  "<" *(%x20-3D / %x3F-7E) ">"
    PARSER_RULE(prose_val, Sequence(CharExact('<'), Repeat(CharPred([] (auto ch) { return (ch >= 0x20 && ch <= 0x3d) || (ch >= 0x3f && ch <= 0x7e); })), CharExact('>')));
    
    // char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
    PARSER_RULE(char_val, Sequence(CharExact('\"'), Repeat(CharPred([] (auto ch) { return ch == 0x20 || ch == 0x21 || (ch >= 0x23 && ch <= 0x7e); })), CharExact('\"')));
    
    // bin-val        =  "b" 1*BIT
    //                   [ 1*("." 1*BIT) / ("-" 1*BIT) ]
    PARSER_RULE(bin_val, Sequence(CharExact('b'), Repeat<1>(BIT()), Optional(Alternatives(Repeat<1>(CharExact('.'), Repeat<1>(BIT())), Sequence(CharExact('-'), Repeat<1>(BIT()))))));

    // dec-val        =  "d" 1*DIGIT
    //                   [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
    PARSER_RULE(dec_val, Sequence(CharExact('d'), Repeat<1>(DIGIT()), Optional(Alternatives(Repeat<1>(CharExact('.'), Repeat<1>(DIGIT())), Sequence(CharExact('-'), Repeat<1>(DIGIT()))))));

    // hex-val        =  "x" 1*HEXDIG
    //                   [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
    PARSER_RULE(hex_val, Sequence(CharExact('x'), Repeat<1>(HEXDIG()), Optional(Alternatives(Repeat<1>(CharExact('.'), Repeat<1>(HEXDIG())), Sequence(CharExact('-'), Repeat<1>(HEXDIG()))))));

    // num-val        =  "%" (bin-val / dec-val / hex-val
    PARSER_RULE(num_val, Sequence(CharExact('%'), Alternatives(bin_val, dec_val, hex_val)));

    // element        =  rulename / group / option /
    //                   char-val / num-val / prose-val
    PARSER_RULE(element, Alternatives(rulename(), group(), option(), char_val(), num_val(), prose_val()));

    // repetition     =  [repeat] element
    PARSER_RULE(repetition, Sequence(Optional(repeat()), element()));

    // concatenation  =  repetition *(1*c-wsp repetition)
    PARSER_RULE(concatenation, Sequence(repetition, Repeat(Repeat<1>(c_wsp), repetition)));

    //alternation    =  concatenation
    //            *(*c-wsp "/" *c-wsp concatenation)
    PARSER_RULE_PARTIAL(alternation, Sequence(concatenation(), Repeat(Repeat(c_wsp()), CharExact('/'), Repeat(c_wsp), concatenation())));

    // elements       =  alternation *c-wsp
    PARSER_RULE(elements, Sequence(alternation(), Repeat(c_wsp())));

    // rule           =  rulename defined-as elements c-nl
    //                            ; continues if next line starts
    //                            ;  with white space
    PARSER_RULE(rule, Sequence(rulename, defined_as, elements, c_nl));

    // rulelist       =  1*( rule / (*c-wsp c-nl) )
    PARSER_RULE(rulelist, Repeat<1>(Alternatives(rule, Sequence(Repeat(c_wsp) c_nl))));

    //
    //repetition     =  [repeat] element
    //
    //repeat         =  1*DIGIT / (*DIGIT "*" *DIGIT)
    //
    //element        =  rulename / group / option /
    //            char-val / num-val / prose-val
    //
    //group          =  "(" *c-wsp alternation *c-wsp ")"
    //
    //option         =  "[" *c-wsp alternation *c-wsp "]"
    //
    //char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
    //                ; quoted string of SP and VCHAR
    //                ;  without DQUOTE
    //
    //num-val        =  "%" (bin-val / dec-val / hex-val)
    //
    //bin-val        =  "b" 1*BIT
    //            [ 1*("." 1*BIT) / ("-" 1*BIT) ]
    //                ; series of concatenated bit values
    //                ;  or single ONEOF range
    //
    //dec-val        =  "d" 1*DIGIT
    //            [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
    //
    //hex-val        =  "x" 1*HEXDIG
    //            [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
    //
    //prose-val      =  "<" *(%x20-3D / %x3F-7E) ">"
    //                ; bracketed string of SP and VCHAR
    //                ;  without angles
    //                ; prose description, to be used as
    //                ;  last resort
}
