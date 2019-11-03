// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// ABNF rules
#pragma once

#include "ParserCore.hpp"

namespace RFC5234ABNF
{
    using namespace RFC5234Core;

    // rulename       =  ALPHA *(ALPHA / DIGIT / "-")
    PARSER_RULE(rulename, Sequence(ALPHA(), Repeat(Alternatives(ALPHA(), DIGIT(), CharVal<'-'>()))));

    // comment        =  ";" *(WSP / VCHAR) CRLF
    PARSER_RULE(comment, Sequence(CharVal<';'>(), Repeat(Alternatives(WSP(), VCHAR())), CRLF()))

    // c-nl           =  comment / CRLF
    //                 ; comment or newline
    PARSER_RULE(c_nl, Alternatives(comment(), CRLF()));

    // c-wsp          =  WSP / (c-nl WSP)
    PARSER_RULE(c_wsp, Alternatives(WSP(), Sequence(c_nl(), WSP())));

    // defined-as     =  *c-wsp ("=" / "=/") *c-wsp
    //                ; basic rules definition and
    //                ;  incremental alternatives
    
    PARSER_RULE(defined_as, Sequence(Repeat(c_wsp()), Alternatives(CharVal<'='>(), Sequence(CharVal<'='>(), CharVal<'/'>())), Repeat(c_wsp())));

    using RepeatData = std::tuple<SubstringPos, std::tuple<SubstringPos, SubstringPos> >;
    enum RepeatFields
    {
        RepeatFields_Fixed,
        RepeatFields_Range
    };
    // repeat         =  1*DIGIT / (*DIGIT "*" *DIGIT)
    PARSER_RULE_CDATA(repeat, RepeatData, Union(Repeat<1>(DIGIT()), Sequence(Repeat(DIGIT()), CharVal<'*'>(), Repeat(DIGIT()))));

    PARSER_RULE_FORWARD(alternation);

    class AlternationData;

    using GroupData = AlternationData;
    // group          =  "(" *c-wsp alternation *c-wsp ")"
    PARSER_RULE(group, Sequence(CharVal<'('>(), Idx<INDEX_NONE>(), Repeat(c_wsp()), Idx<INDEX_THIS>(), alternation(), Idx<INDEX_NONE>(), Repeat(c_wsp()), CharVal<')'>()));

    using OptionData = AlternationData;
    // option         =  "[" *c-wsp alternation *c-wsp "]"
    PARSER_RULE(option, Sequence(CharVal<'['>(), Idx<INDEX_NONE>(),Repeat(c_wsp()), Idx<INDEX_THIS>(), alternation(), Idx<INDEX_NONE>(),Repeat(c_wsp()), CharVal<']'>()));

    // prose-val      =  "<" *(%x20-3D / %x3F-7E) ">"
    PARSER_RULE(prose_val, Sequence(CharVal<'<'>(), Repeat(Alternatives(CharRange<0x20, 0x3D>(), CharRange<0x3F, 0x7E>())), CharVal<'>'>()));
    
    // char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
    PARSER_RULE(char_val, Sequence(CharVal<'\"'>(), Repeat(Alternatives(CharRange<0x20, 0x21>(), CharRange<0x23, 0x7E>())), CharVal<'\"'>()));
    
    using NumValSpecData = std::tuple<SubstringPos, std::vector<SubstringPos>, SubstringPos >;
    enum NumValSpecFields
    {
        NumValSpecFields_FirstValue,
        NumValSpecFields_SequenceValues,
        NumValSpecFields_RangeLastValue
    };

    // bin-val        =  "b" 1*BIT
    //                   [ 1*("." 1*BIT) / ("-" 1*BIT) ]
    PARSER_RULE(bin_val, Sequence(CharVal<'b'>(), Idx<NumValSpecFields_FirstValue>(), Repeat<1>(BIT()),
        Idx<INDEX_THIS>(), Optional(Union(Idx<NumValSpecFields_SequenceValues>(), Repeat<1>(CharVal<'.'>(), Repeat<1>(BIT())), Idx<INDEX_THIS>(), Sequence(CharVal<'-'>(), Idx<NumValSpecFields_RangeLastValue>(), Repeat<1>(BIT()))))));

    // dec-val        =  "d" 1*DIGIT
    //                   [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
    PARSER_RULE(dec_val, Sequence(CharVal<'d'>(), Idx<NumValSpecFields_FirstValue>(), Repeat<1>(DIGIT()),
        Idx<INDEX_THIS>(), Optional(Union(Idx<NumValSpecFields_SequenceValues>(), Repeat<1>(CharVal<'.'>(), Repeat<1>(DIGIT())), Idx<INDEX_THIS>(), Sequence(CharVal<'-'>(), Idx<NumValSpecFields_RangeLastValue>(), Repeat<1>(DIGIT()))))));

    // hex-val        =  "x" 1*HEXDIG
    //                   [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
    PARSER_RULE(hex_val, Sequence(CharVal<'x'>(), Idx<NumValSpecFields_FirstValue>(), Repeat<1>(HEXDIG()),
        Idx<INDEX_THIS>(), Optional(Union(Idx<NumValSpecFields_SequenceValues>(), Repeat<1>(CharVal<'.'>(), Repeat<1>(HEXDIG())), Idx<INDEX_THIS>(), Sequence(CharVal<'-'>(), Idx<NumValSpecFields_RangeLastValue>(), Repeat<1>(HEXDIG()))))));

    using NumValData = std::tuple<NumValSpecData, NumValSpecData, NumValSpecData>;
    enum NumValFields
    {
        NumValFields_Bin,
        NumValFields_Dec,
        NumValFields_Hex
    };

    // num-val        =  "%" bin-val / dec-val / hex-val
    PARSER_RULE_CDATA(num_val, NumValData, Sequence(CharVal<'%'>(), Union(bin_val(), dec_val(), hex_val())));

    using ElementData = std::tuple<SubstringPos, GroupData, OptionData, SubstringPos, NumValData, SubstringPos>;
    enum ElementFields
    {
        ElementFields_Rulename,
        ElementFields_Group,
        ElementFields_Option,
        ElementFields_CharVal,
        ElementFields_NumVal,
        ElementFields_ProseVal
    };

    // element        =  rulename / group / option /
    //                   char-val / num-val / prose-val
    PARSER_RULE_CDATA(element, ElementData, Union(rulename(), group(), option(), char_val(), num_val(), prose_val()));

    using RepetitionData = std::tuple<RepeatData, ElementData>;
    enum RepetitionFields
    {
        RepetitionFields_Repeat,
        RepetitionFields_Element
    };
    // repetition     =  [repeat] element
    PARSER_RULE_CDATA(repetition, RepetitionData, Sequence(Optional(repeat()), element()));

    class ConcatenationData : public std::vector<RepetitionData>
    {
    };

    // concatenation  =  repetition *(1*c-wsp repetition)
    PARSER_RULE_CDATA(concatenation, ConcatenationData, HeadTail(repetition(), Idx<INDEX_NONE>(), Repeat<1>(c_wsp()), Idx<INDEX_THIS>(), repetition()));

    class AlternationData : public std::vector<ConcatenationData>
    {
    };

    // alternation    =  concatenation
    //                  *(*c-wsp "/" *c-wsp concatenation)
    PARSER_RULE_PARTIAL(alternation,
        HeadTail(concatenation(), Idx<INDEX_NONE>(), Repeat(c_wsp()), CharVal<'/'>(), Idx<INDEX_NONE>(), Repeat(c_wsp()), Idx<INDEX_THIS>(), concatenation()));

    using ElementsData = AlternationData;
    // elements       =  alternation *c-wsp
    PARSER_RULE(elements, Sequence(Idx<INDEX_THIS>(), alternation(), Idx<INDEX_NONE>(), Repeat(c_wsp())));

    using RuleData = std::tuple<SubstringPos, SubstringPos, ElementsData>;
    enum RuleFields
    {
        RuleFields_Rulename,
        RuleFields_DefinedAs,
        RuleFields_Elements
    };
    // rule           =  rulename defined-as elements c-nl
    //                            ; continues if next line starts
    //                            ;  with white space
    PARSER_RULE_CDATA(rule, RuleData, Sequence(rulename(), defined_as(), elements(), Idx<INDEX_NONE>(), c_nl()));

    using RuleListData = std::vector<RuleData>;
    // rulelist       =  1*( rule / (*c-wsp c-nl) )
    PARSER_RULE_CDATA(rulelist, RuleListData, Repeat<1>(Alternatives(rule(), Idx<INDEX_NONE>(), Sequence(Repeat(c_wsp()), c_nl()))));
}
