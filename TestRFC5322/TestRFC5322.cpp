// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234

#include <string>
#include <iostream>

#include "ParserIO.hpp"
#include "rfc5322/RFC5322Rules.hpp"

class PrintVisitor
{
    static size_t const IndentationSpacing = 4;
    size_t indentation_ = 0;
    std::vector<char> const & buffer_;
public:
    inline PrintVisitor(std::vector<char> const & buffer)
    : buffer_(buffer)
    {

    }

    template <typename MEMBER_INFO, typename CONTINUATION>
    inline void OnMember(MEMBER_INFO const & memberInfo, SubstringPos memberValue, CONTINUATION && continuation)
    {
        std::cout << std::string(indentation_, ' ') << memberInfo.GetName() << ": " << ToString(buffer_, memberValue) << std::endl;
    }

    template <typename MEMBER_INFO, typename TYPE, typename CONTINUATION>
    inline void OnMember(MEMBER_INFO const & memberInfo, TYPE const & memberValue, CONTINUATION && continuation)
    {
        std::cout << std::string(indentation_, ' ') << memberInfo.GetName() << ":" << std::endl;
        indentation_ += IndentationSpacing;
        continuation();
        indentation_ -= IndentationSpacing;
    }
};


void test_address(std::string const & addr)
{
    std::cout << addr;

    auto parser(Make_ParserFromString(addr));
    using namespace RFC5322;

    MailboxData mailbox;
    ParseExact(parser, &mailbox);

    AddressListData addresses;
    if (ParseExact(parser, &addresses))
    {
        std::cout << " is OK\n";
        std::cout << addresses.size() << " address" << (addresses.size() > 1 ? "es" : "") << " parsed." << std::endl;

        for (AddressData const & address : addresses)
        {
            NamedTuple::Visit(address, PrintVisitor(parser.OutputBuffer()));

            continue;
        }
    }

    //for (auto const & parseError : parser.Errors())
    //{
    //    std::cerr << "Parsing error at " << std::get<0>(parseError) << ":" << std::get<1>(parseError) << std::endl;
    //}

    std::cout << std::endl;
}

int main(int argc, char ** argv)
{
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

    return EXIT_SUCCESS;
}