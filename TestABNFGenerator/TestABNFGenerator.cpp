// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//

#define PARSER_LF_AS_CRLF

#include "ParserIO.hpp"
#include "rfc5234/ABNFParserGenerator.hpp"

int main(int argc, char ** argv)
{
    std::string str(R"ABNF(
char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
                    ; quoted string of SP and VCHAR
                    ;  without DQUOTE
defined-as     =  *c-wsp ("=" / "=/") *c-wsp
                    ; basic rules definition and
                    ;  incremental alternatives

elements       =  alternation *c-wsp

c-wsp          =  WSP / (c-nl WSP)

c-nl           =  comment / CRLF
                    ; comment or newline

comment        =  ";" *(WSP / VCHAR) CRLF

alternation    =  concatenation
                *(*c-wsp "/" *c-wsp concatenation)

concatenation  =  repetition *(1*c-wsp repetition)

repetition     =  [repeat] element

repeat         =  1*DIGIT / (*DIGIT "*" *DIGIT)
repeat =/ DIGIT ; Added to test =/

element        =  rulename / group / option /
                char-val / num-val / prose-val

group          =  "(" *c-wsp alternation *c-wsp ")"

option         =  "[" *c-wsp alternation *c-wsp "]"

char-val       =  DQUOTE *(%x20-21 / %x23-7E) DQUOTE
                    ; quoted string of SP and VCHAR
                    ;  without DQUOTE

num-val        =  "%" (bin-val / dec-val / hex-val)

bin-val        =  "b" 1*BIT
                [ 1*("." 1*BIT) / ("-" 1*BIT) ]
                    ; series of concatenated bit values
                    ;  or single ONEOF range

dec-val        =  "d" 1*DIGIT
                [ 1*("." 1*DIGIT) / ("-" 1*DIGIT) ]

hex-val        =  "x" 1*HEXDIG
                [ 1*("." 1*HEXDIG) / ("-" 1*HEXDIG) ]

prose-val      =  "<" *(%x20-3D / %x3F-7E) ">"
                    ; bracketed string of SP and VCHAR
                    ;  without angles
                    ; prose description, to be used as
                    ;  last resort
)ABNF");

    using namespace RFC5234ABNF;

    std::cout << "Parsing ABNF rules... ";

    auto parser(Make_ParserFromString(str));
    RuleListData rules;
    if (ParseExact(parser, &rules))
    {
        std::cout << "Success" << std::endl;
        GenerateABNFParser(std::cout, rules, parser.OutputBuffer());
    }
    else
    {
        std::cout << "Failure" << std::endl;
    }

    for (auto const & parseError : parser.Errors())
    {
        parseError(std::cerr, "");
    }

    return EXIT_SUCCESS;
}