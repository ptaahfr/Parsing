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
    PARSER_RULE(defined_as, Sequence(Repeat(c_wsp()), Alternatives(CharExact('='), Sequence(CharExact('='), CharExact('/'))), Repeat(c_wsp())));

    using RepeatData = std::tuple<SubstringPos, std::tuple<SubstringPos, SubstringPos> >;
    enum RepeatFields
    {
        RepeatFields_Fixed,
        RepeatFields_Range
    };
    // repeat         =  1*DIGIT / (*DIGIT "*" *DIGIT)
    PARSER_RULE(repeat, Alternatives(Repeat<1>(DIGIT()), Sequence(Repeat(DIGIT()), CharExact('*'), Repeat(DIGIT()))));

    PARSER_RULE_FORWARD(alternation);

    class AlternationData;

    using GroupData = AlternationData;
    // group          =  "(" *c-wsp alternation *c-wsp ")"
    PARSER_RULE(group, IndexedSequence(
        std::index_sequence<INDEX_NONE, INDEX_NONE, INDEX_THIS, INDEX_NONE, INDEX_NONE>(),
        CharExact('('), Repeat(c_wsp()), alternation(), Repeat(c_wsp()), CharExact(')')));

    using OptionData = AlternationData;
    // option         =  "[" *c-wsp alternation *c-wsp "]"
    PARSER_RULE(option, Sequence(CharExact('['), Repeat(c_wsp()), alternation(), Repeat(c_wsp()), CharExact(']')));

    // prose-val      =  "<" *(%x20-3D / %x3F-7E) ">"
    PARSER_RULE(prose_val, IndexedSequence(
        std::index_sequence<INDEX_NONE, INDEX_THIS, INDEX_NONE>(),
        CharExact('<'), Repeat(CharPred([] (auto ch) { return (ch >= 0x20 && ch <= 0x3d) || (ch >= 0x3f && ch <= 0x7e); })), CharExact('>')));
    
    // char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
    PARSER_RULE(char_val, IndexedSequence(
        std::index_sequence<INDEX_NONE, INDEX_THIS, INDEX_NONE>(),
        CharExact('\"'), Repeat(CharPred([] (auto ch) { return ch == 0x20 || ch == 0x21 || (ch >= 0x23 && ch <= 0x7e); })), CharExact('\"')));
    
    using NumValSpecData = std::tuple<SubstringPos, std::vector<SubstringPos>, SubstringPos >;
    enum NumValSpecFields
    {
        NumValSpecFields_FirstValue,
        NumValSpecFields_SequenceValues,
        NumValSpecFields_RangeLastValue
    };

    // bin-val        =  "b" 1*BIT
    //                   [ 1*("." 1*BIT) / ("-" 1*BIT) ]
    PARSER_RULE(bin_val, IndexedSequence(
        std::index_sequence<INDEX_NONE, NumValSpecFields_FirstValue, INDEX_THIS>(),
        CharExact('b'), Repeat<1>(BIT()), Optional(
            IndexedAlternatives(std::index_sequence<NumValSpecFields_SequenceValues, NumValSpecFields_RangeLastValue>(),
                Repeat<1>(CharExact('.'), Repeat<1>(BIT())), Sequence(CharExact('-'), Repeat<1>(BIT()))))));

    // dec-val        =  "d" 1*DIGIT
    //                   [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]
    PARSER_RULE(dec_val, IndexedSequence(
        std::index_sequence<INDEX_NONE, NumValSpecFields_FirstValue, INDEX_THIS>(),
        CharExact('d'), Repeat<1>(DIGIT()), Optional(
        IndexedAlternatives(std::index_sequence<NumValSpecFields_SequenceValues, NumValSpecFields_RangeLastValue>(),
            Repeat<1>(CharExact('.'), Repeat<1>(DIGIT())), Sequence(CharExact('-'), Repeat<1>(DIGIT()))))));

    // hex-val        =  "x" 1*HEXDIG
    //                   [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]
    PARSER_RULE(hex_val, IndexedSequence(
        std::index_sequence<INDEX_NONE, NumValSpecFields_FirstValue, INDEX_THIS>(),
        CharExact('x'), Repeat<1>(HEXDIG()), Optional(
        IndexedAlternatives(std::index_sequence<NumValSpecFields_SequenceValues, NumValSpecFields_RangeLastValue>(),
            Repeat<1>(CharExact('.'), Repeat<1>(HEXDIG())), Sequence(CharExact('-'), Repeat<1>(HEXDIG()))))));

    using NumValData = std::tuple<NumValSpecData, NumValSpecData, NumValSpecData>;
    enum NumValFields
    {
        NumValFields_Bin,
        NumValFields_Dec,
        NumValFields_Hex
    };

    // num-val        =  "%" bin-val / dec-val / hex-val
    PARSER_RULE(num_val, IndexedSequence(
        std::index_sequence<INDEX_NONE, INDEX_THIS>(),
        CharExact('%'), IndexedAlternatives(
            std::index_sequence<NumValFields_Bin, NumValFields_Dec, NumValFields_Hex>(),
            bin_val(), dec_val(), hex_val())));

    using ElementData = std::tuple<SubstringPos, GroupData, OptionData, SubstringPos, NumValData>;
    enum ElementFields
    {
        ElementFields_Rulename,
        ElementFields_Group,
        ElementFields_Option,
        ElementFields_CharOrProse,
        ElementFields_NumVal
    };

    // element        =  rulename / group / option /
    //                   char-val / num-val / prose-val
    PARSER_RULE(element, IndexedAlternatives(
        std::index_sequence<ElementFields_Rulename, ElementFields_Group, ElementFields_Option, ElementFields_CharOrProse, ElementFields_NumVal, ElementFields_CharOrProse>(),
        rulename(), group(), option(), char_val(), num_val(), prose_val()));

    using RepetitionData = std::tuple<RepeatData, ElementData>;
    enum RepetitionFields
    {
        RepetitionFields_Repeat,
        RepetitionFields_Element
    };
    // repetition     =  [repeat] element
    PARSER_RULE(repetition, Sequence(Optional(repeat()), element()));

    using ConcatenationData = std::vector<RepetitionData>;
    // concatenation  =  repetition *(1*c-wsp repetition)
    PARSER_RULE(concatenation, IndexedSequence(
        std::index_sequence<INDEX_THIS, INDEX_THIS>(),
        Head(repetition()),
        Repeat(
            IndexedSequence(std::index_sequence<INDEX_NONE, INDEX_THIS>(),
            Repeat<1>(c_wsp()), repetition()))));

    class AlternationData : public std::vector<ConcatenationData>
    {
    };
    // alternation    =  concatenation
    //                  *(*c-wsp "/" *c-wsp concatenation)
    PARSER_RULE_PARTIAL(alternation, IndexedSequence(
        std::index_sequence<INDEX_THIS, INDEX_THIS>(),
        Head(concatenation()),
        Repeat(
            IndexedSequence(std::index_sequence<INDEX_NONE, INDEX_NONE, INDEX_NONE, INDEX_THIS>(),
            Repeat(c_wsp()), CharExact('/'), Repeat(c_wsp()), concatenation()))));

    using ElementsData = AlternationData;
    // elements       =  alternation *c-wsp
    PARSER_RULE(elements, IndexedSequence(
        std::index_sequence<INDEX_THIS, INDEX_NONE>(),
        alternation(), Repeat(c_wsp())));

    using RuleData = std::tuple<SubstringPos, ElementsData>;
    enum RuleFields
    {
        RuleFields_Rulename,
        RuleFields_Elements
    };
    // rule           =  rulename defined-as elements c-nl
    //                            ; continues if next line starts
    //                            ;  with white space
    PARSER_RULE(rule, IndexedSequence(
        std::index_sequence<RuleFields_Rulename, INDEX_NONE, RuleFields_Elements, INDEX_NONE>(),
        rulename(), defined_as(), elements(), c_nl()));

    using RuleListData = std::vector<RuleData>;
    // rulelist       =  1*( rule / (*c-wsp c-nl) )
    PARSER_RULE(rulelist, Repeat<1>(IndexedAlternatives(
        std::index_sequence<INDEX_THIS, INDEX_NONE>(),
        rule(), Sequence(Repeat(c_wsp()), c_nl()))));
}
