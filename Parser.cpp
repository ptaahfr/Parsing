// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234

#ifdef _MSC_VER
#pragma warning(disable: 4503)
#endif

#define PARSER_LF_AS_CRLF

#include "ParserBase.hpp"
#include "ParserCore.hpp"
#include <algorithm>

#include "ParserRFC5322Data.hpp"
#include "ParserRFC5322Rules.hpp"
#include "ParserRFC5234.hpp"

#include <iostream>

#define TEST_RULE(type, name, str) \
{ \
    type nameResult; \
    auto parser(Make_ParserFromString(std::string(str))); \
    std::cout << "Parsing rule " << #name; \
    if (!ParseExact(parser, &nameResult, name())) \
        std::cout<< " failed" << std::endl; \
    else \
        std::cout << " succeed" << std::endl; \
}

void TestRFC5234()
{
    using namespace RFC5234ABNF;
    TEST_RULE(SubstringPos, defined_as, "=/");
    TEST_RULE(SubstringPos, rulename, "defined-as");
    TEST_RULE(SubstringPos, comment, "; test comment\r\n");
    TEST_RULE(SubstringPos, c_nl, "; test comment\r\n");
    TEST_RULE(SubstringPos, c_wsp, "; test comment\r\n ");
    TEST_RULE(SubstringPos, defined_as, " ; test comment \r\n =");
    TEST_RULE(SubstringPos, defined_as, " ; test comment \r\n =/ ; test comment \r\n ");
    TEST_RULE(RepeatData, repeat, "1*5");
    TEST_RULE(RepeatData, repeat, "540");
    TEST_RULE(AlternationData, alternation, "rule1 / rule2 / rule3");
    TEST_RULE(ElementsData, elements, "*c-wsp");
    TEST_RULE(ElementsData, elements, "*c-wsp \"/\"");
    TEST_RULE(ElementsData, elements, "*c-wsp \"/\" *c-wsp concatenation");
    TEST_RULE(ElementsData, elements, "*(*c-wsp \"/\" *c-wsp concatenation)");
    TEST_RULE(ElementsData, elements, "concatenation\r\n *(*c-wsp \"/\" *c-wsp concatenation)");
    TEST_RULE(ElementsData, elements, "concatenation\r\n *(<test <\"string>)");
    TEST_RULE(RuleData, rule, "rule1 = concatenation\r\n *(<test <\"string>)\r\n");
    TEST_RULE(SubstringPos, c_nl, "\r\n");
    TEST_RULE(RuleData, rule, R"ABNF(defined-as     =  *c-wsp
)ABNF");
}

#include <map>
#include <set>
#include <sstream>

void GenerateParser(std::ostream & os, RFC5234ABNF::RuleListData const & rules, std::vector<char> const & buffer)
{
    using namespace RFC5234ABNF;

    std::map<std::string, std::tuple<std::list<std::string>, std::string> > mapRules;

    auto transformRulename = [&] (auto const & ruleNameField)
    {
        auto ruleName(ToString(buffer, ruleNameField));
        std::transform(ruleName.begin(), ruleName.end(), ruleName.begin(), [](char ch) { return ch == '-' ? '_' : ch; } );
        return ruleName;
    };

    for (auto const & rule : rules)
    {
        std::stringstream cout; // << "PARSER_RULE(" << transformRulename(std::get<RuleFields_Rulename>(rule)) << ", ";
        std::list<std::string> dependencies;
        std::function<void(AlternationData const &)> generateAlternation;

        auto generateChar = [&](auto value, char const * prefix)
        {
            cout << "CharVal<" << prefix << ToString(buffer, value) << ">()";
        };

        auto generateNumValSpec = [&](NumValSpecData const & numValSpec, char const * prefix)
        {
            auto const & firstValue(std::get<NumValSpecFields_FirstValue>(numValSpec));
            auto const & seqValues(std::get<NumValSpecFields_SequenceValues>(numValSpec));
            auto const & rangeLastValue(std::get<NumValSpecFields_RangeLastValue>(numValSpec));

            if (!IsNull(seqValues))
            {
                cout << "Sequence(";
                generateChar(firstValue, prefix);
                for (auto const & seqValue : seqValues)
                {
                    cout << ", ";
                    generateChar(seqValue, prefix);
                }
                cout << ")";
            }
            else if (!IsNull(rangeLastValue))
            {
                cout << "CharRange<" << prefix << ToString(buffer, firstValue) << ", " << prefix << ToString(buffer, rangeLastValue) << ">()";
            }
            else
            {
                generateChar(firstValue, prefix);
            }
        };

        auto generateNumVal = [&](NumValData const & numVal)
        {
            auto const & hexVal(std::get<NumValFields_Hex>(numVal));
            if (!IsNull(hexVal))
            {
                generateNumValSpec(hexVal, "0x");
            }
            else
            {
                auto const & binVal(std::get<NumValFields_Bin>(numVal));
                if (!IsNull(binVal))
                {
                    generateNumValSpec(binVal, "0b");
                }
                else
                {
                    generateNumValSpec(std::get<NumValFields_Dec>(numVal), "");
                }
            }
        };

        auto generateElement = [&](ElementData const & element, bool insideRepeat)
        {
            auto charValOrProseVal(std::get<ElementFields_CharVal>(element));
            if (IsNull(charValOrProseVal))
                charValOrProseVal = std::get<ElementFields_ProseVal>(element);
            if (!IsNull(charValOrProseVal))
            {
                // Skip delimiters <> or ""
                size_t length(std::get<1>(charValOrProseVal) - std::get<0>(charValOrProseVal));
                if (length > 2)
                {
                    if (length > 3 && !insideRepeat)
                    {
                        cout << "Sequence(";
                    }

                    bool first = true;
                    for (size_t t = std::get<0>(charValOrProseVal) + 1; t < std::get<1>(charValOrProseVal) - 1; ++t)
                    {
                        if (!first)
                            cout << ", ";
                        first = false;

                        cout << "CharVal<0x" << std::hex << std::setfill('0') << std::setw(2) << (MaxCharType)buffer[t] << ">()";
                    }

                    if (length > 3 && !insideRepeat)
                    {
                        cout << ")";
                    }
                }
            }
            else
            {
                auto const & numVal(std::get<ElementFields_NumVal>(element));
                if (!IsNull(numVal))
                {
                    generateNumVal(numVal);
                }
                else
                {
                    auto const & group(std::get<ElementFields_Group>(element));
                    if (!IsNull(group))
                    {
                        generateAlternation(group);
                    }
                    else
                    {
                        auto const & option(std::get<ElementFields_Option>(element));
                        if (!IsNull(option))
                        {
                            cout << "Optional(";
                            generateAlternation(option);
                            cout << ")";
                        }
                        else
                        {
                            std::string otherRuleName(transformRulename(std::get<ElementFields_Rulename>(element)));
                            dependencies.push_back(otherRuleName);
                            cout << otherRuleName << "()";
                        }
                    }
                }
            }
        };

        auto generateRepetition = [&](RepetitionData const & repetition)
        {
            auto const & repeat(std::get<RepetitionFields_Repeat>(repetition));
            auto const & element(std::get<RepetitionFields_Element>(repetition));

            bool insideRepeat(!IsNull(repeat));

            if (insideRepeat)
            {
                cout << "Repeat";

                if (!IsEmpty(repeat))
                {
                    auto const & fixed(std::get<RepeatFields_Fixed>(repeat));
                    auto const & range(std::get<RepeatFields_Range>(repeat));

                    cout << "<";

                    if (!IsEmpty(fixed))
                    {
                        auto num(ToString(buffer, fixed));
                        cout << num << ", " << num << ">(";
                    }
                    else if (IsEmpty(std::get<0>(range)))
                    {
                        cout << "0, " << ToString(buffer, std::get<1>(range));
                    }
                    else if (IsEmpty(std::get<1>(range)))
                    {
                        cout << ToString(buffer, std::get<0>(range));
                    }
                    else
                    {
                        cout << ToString(buffer, std::get<0>(range)) << ", " << ToString(buffer, std::get<1>(range)) << ">(";
                    }

                    cout << ">";
                }

                cout << "(";
            }

            generateElement(element, insideRepeat);

            if (insideRepeat)
            {
                cout << ")";
            }
        };

        auto generateConcatenation = [&](ConcatenationData const & concatenation)
        {
            if (concatenation.size() == 1)
            {
                generateRepetition(concatenation.front());
            }
            else
            {
                bool first = true;
                cout << "Sequence(";
                for (auto const & repetition : concatenation)
                {
                    if (!first)
                        cout << ", ";
                    first = false;
                    generateRepetition(repetition);
                }
                cout << ")";
            }
            // cout << 
        };

        generateAlternation = [&](AlternationData const & alternation)
        {
            if (alternation.size() == 1)
            {
                generateConcatenation(alternation.front());
            }
            else
            {
                bool first = true;
                cout << "Alternatives(";
                for (auto const & concatenation : alternation)
                {
                    if (!first)
                        cout << ", ";
                    first = false;
                    generateConcatenation(concatenation);
                }
                cout << ")";
            }
        };

        generateAlternation(std::get<RuleFields_Elements>(rule));

        mapRules[transformRulename(std::get<RuleFields_Rulename>(rule))] = std::make_tuple(std::move(dependencies), cout.str());
    }

    std::set<std::string> parsingRules, parsedRules, parsedRulesForward;
    std::function<void(std::string const &)> outputRule;
    outputRule = [&](std::string const & ruleName)
    {
        if (parsedRules.find(ruleName) != parsedRules.end())
            return;
        if (parsingRules.find(ruleName) != parsingRules.end())
        {
            if (parsedRulesForward.find(ruleName) == parsedRulesForward.end())
            {
                os << "PARSER_RULE_FORWARD(" << ruleName << ");" << std::endl << std::endl;
                parsedRulesForward.insert(ruleName);
            }
        }
        else
        {
            parsingRules.insert(ruleName);

            auto ruleEntryPtr = mapRules.find(ruleName);
            if (mapRules.end() != ruleEntryPtr)
            {
                for (auto const & dep : std::get<0>(ruleEntryPtr->second))
                {
                    outputRule(dep);
                }

                if (parsedRulesForward.find(ruleName) != parsedRulesForward.end())
                {
                    os << "PARSER_RULE_PARTIAL(";
                }
                else
                {
                    os << "PARSER_RULE(";
                }

                os << ruleName << ", " << std::get<1>(ruleEntryPtr->second) << ")" << std::endl << std::endl;
            }
            parsedRules.insert(ruleName);
        }

    };
    for (auto const & rule : mapRules)
    {
        outputRule(rule.first);
    }
}

void ParseABNF()
{
    std::string str = R"ABNF(
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
)ABNF";
    using namespace RFC5234ABNF;

    std::cout << "Parsing ABNF rules... ";

    auto parser(Make_ParserFromString(str));
    RuleListData rules;
    if (ParseExact(parser, &rules))
    {
        std::cout << "Success" << std::endl;
        GenerateParser(std::cout, rules, parser.OutputBuffer());
    }
    else
    {
        std::cout << "Failure" << std::endl;
    }

    for (auto const & parseError : parser.Errors())
    {
        parseError(std::cerr, "");
    }

    return;
}

void TestRFC5322()
{
#if 0
#define TEST_RULE(type, name, str) \
    { \
        type nameResult; \
        auto parser(Make_ParserFromString(std::string(str))); \
        if (!Parse(parser, &nameResult, name()) || !parser.Ended()) \
            std::cout << "Parsing rule " << #name << " failed" << std::endl; \
        else \
            std::cout << "Parsing rule " << #name << " succeed" << std::endl; \
    }

    using namespace RFC5322;

    TEST_RULE(TextWithCommData, DotAtom, "local");
    TEST_RULE(AddrSpecData, AddrSpec, "local@domain");
    TEST_RULE(AngleAddrData, AngleAddr, "<local@domain> (comment)");
    TEST_RULE(TextWithCommData, Atom, "bllabla ");
    TEST_RULE(MultiTextWithCommData, DisplayName, "bllabla (comment) blabla");
    TEST_RULE(NameAddrData, NameAddr, "mrs johns <local@domain> (comment)");

    return;
#endif
}

void test_address(std::string const & addr)
{
    std::cout << addr;

    auto parser(Make_ParserFromString(addr));
    using namespace RFC5322;

    AddressListData addresses;
    if (ParseExact(parser, &addresses))
    {
        auto const & outBuffer = parser.OutputBuffer();
        std::cout << " is OK\n";
        std::cout << addresses.size() << " address" << (addresses.size() > 1 ? "es" : "") << " parsed." << std::endl;
        for (AddressData const & address : addresses)
        {
            auto displayAddress = [&](AddrSpecData const & addrSpec, auto indent)
            {
                std::cout << indent << "     Adress: " << std::endl;
                std::cout << indent << "       Local-Part: '" << ToString(outBuffer, false, std::get<AddrSpecFields_LocalPart>(addrSpec)) << "'" << std::endl;
                std::cout << indent << "       Domain-Part: '" << ToString(outBuffer, false, std::get<AddrSpecFields_DomainPart>(addrSpec)) << "'" << std::endl;
            };

            auto displayMailBox = [&](MailboxData const & mailbox, auto indent)
            {
                std::cout << indent << "   Mailbox:" << std::endl;
                if (IsEmpty(std::get<MailboxFields_AddrSpec>(mailbox)))
                {
                    auto const & nameAddrData = std::get<MailboxFields_NameAddr>(mailbox);
                    std::cout << indent << "     Display Name: '" << ToString(outBuffer, true, std::get<NameAddrFields_DisplayName>(nameAddrData)) << std::endl;
                    displayAddress(std::get<AngleAddrFields_Content>(std::get<NameAddrFields_Address>(nameAddrData)), indent);
                }
                else
                {
                    displayAddress(std::get<MailboxFields_AddrSpec>(mailbox), indent);
                }
            };

            if (false == IsEmpty(std::get<AddressFields_Mailbox>(address)))
            {
                displayMailBox(std::get<AddressFields_Mailbox>(address), "");
            }
            else
            {
                auto const & group = std::get<AddressFields_Group>(address);
                std::cout << "   Group:" << std::endl;
                std::cout << "     Display Name: '" << ToString(outBuffer, true, std::get<GroupFields_DisplayName>(group)) << "'" << std::endl;
                std::cout << "     Members:" << std::endl;
                for (auto const & groupAddr : std::get<GroupListFields_Mailboxes>(std::get<GroupFields_GroupList>(group)))
                {
                    displayMailBox(groupAddr, "   ");
                }
            }
        }
    }
    else
    {
        std::cout << " is NOK\n";
    }
    
    //for (auto const & parseError : parser.Errors())
    //{
    //    std::cerr << "Parsing error at " << std::get<0>(parseError) << ":" << std::get<1>(parseError) << std::endl;
    //}

    std::cout << std::endl;
}

int main()
{
    //TestRFC5322();
    //TestRFC5234();

    ParseABNF();

    test_address("troll@bitch.com, arobar     d <sigma@addr.net>, sir john snow <user.name+tag+sorting@example.com(comment)>");
    test_address("arobar     d <sigma@addr.net>");
    test_address("troll@bitch.com");
    test_address("display <simple@example.com>");

    test_address("A@b@c@example.com");

    test_address("simple@example.com");
    test_address("simple(comm1)@(comm2)example.com");
    test_address("very.common@example.com");
    test_address("disposable.style.email.with+symbol@example.com");
    test_address("other.email-with-hyphen@example.com");
    test_address("fully-qualified-domain@example.com");
    test_address("user.name+tag+sorting@example.com");
    test_address("x@example.com");
    test_address("example-indeed@strange-example.com");
    test_address("admin@mailserver1");
    test_address("example@s.example");
    test_address("\" \"@example.org");
    test_address("\"john..doe\"@example.org");
    test_address("\"john..doe\"@example.org, friends: rantanplan@lucky, titi@disney, dingo@disney;");

    test_address("Abc.example.com");
    test_address("a\"b(c)d,e:f;g<h>i[j\\k]l@example.com");
    test_address("just\"not\"right@example.com");
    test_address("this is\"not\\allowed@example.com");
    test_address("this\\ still\\\"not\\\\allowed@example.com");
    test_address("1234567890123456789012345678901234567890123456789012345678901234+x@example.com");

    return 0;
}