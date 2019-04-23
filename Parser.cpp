// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234

#ifdef _MSC_VER
#pragma warning(disable: 4503)
#endif

#include "ParserBase.hpp"
#include "ParserCore.hpp"
#include <algorithm>

#include "ParserRFC5322Data.hpp"
#include "ParserRFC5322Rules.hpp"

#ifdef TEST_ABNF
#include "ParserRFC5234.hpp"
#endif

#include <iostream>

void ParseABNF()
{
#ifdef TEST_ABNF
    std::string ABNFRulesABNF = R"ABNF(
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

    auto parser(Make_Parser([&, pos = (size_t)0] () mutable
    {
        if (pos < ABNFRulesABNF.size())
            return (int)ABNFRulesABNF[pos++];
        return EOF;
    }, (char)0));

    RuleListData rules;
    Parse(parser, &rules, rulelist());
#endif
}

void test_grammar()
{
    using namespace RFC5234Core;

    //std::cout << IsConstant(CR()) << std::endl;
    //std::cout << IsConstant(Sequence(CR())) << std::endl;
    //std::cout << IsConstant(Sequence(Repeat<10, 10>(CRLF()))) << std::endl;
    //std::cout << IsConstant(Sequence(Repeat<1>(DIGIT()))) << std::endl;

    return;
}

void TestRFC5322()
{
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
}

void test_address(std::string const & addr)
{
    std::cout << addr;

    auto parser(Make_ParserFromString(addr));

    using namespace RFC5322;

    AddressListData addresses;
    if (Parse(parser, &addresses) && parser.Ended())
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

    std::cout << std::endl;
}

int main()
{
    TestRFC5322();

    test_grammar();

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