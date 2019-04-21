// test parsing code for email adresses based on RFC 5322 & 5234
// (c) Ptaah

#include <cstdint>
#include <type_traits>
#include <vector>
#include <exception>
#include <sstream>
#include <algorithm>
#include <cassert>

using SubstringPos = std::pair<size_t, size_t>;

struct TextWithComm
{
    SubstringPos CommentBefore = SubstringPos(0, 0);
    SubstringPos Content = SubstringPos(0, 0);
    SubstringPos CommentAfter = SubstringPos(0, 0);
};

using MultiTextWithComm = std::vector<TextWithComm>;

struct AddrSpec
{
    TextWithComm LocalPart;
    TextWithComm DomainPart;
};

struct AngleAddr
{
    SubstringPos CommentBefore;
    AddrSpec Content;
    SubstringPos CommentAfter;
};

struct Mailbox
{
    MultiTextWithComm DisplayName;
    AngleAddr Address;
};

using MailboxList = std::vector<Mailbox>;

struct Group
{
    MultiTextWithComm DisplayName;
    MailboxList Members;
    SubstringPos Comment;
};

struct Address
{
    Mailbox Mailbox;
    Group Group;
};

using AddressList = std::vector<Address>;

template <typename CHAR_TYPE>
std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, SubstringPos subString)
{
    subString.first = std::min(subString.first, buffer.size());
    subString.second = std::max(subString.first, std::min(subString.second, buffer.size()));
    return std::basic_string<CHAR_TYPE>(buffer.data() + subString.first, buffer.data() + subString.second);
}

template <typename CHAR_TYPE>
std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, bool withComments, TextWithComm const & text)
{
    std::basic_string<CHAR_TYPE> result;

#if 1
    if (withComments)
    {
        result += ToString(buffer, SubstringPos(text.CommentBefore.first, text.CommentAfter.second));
    }
    else
    {
        result += ToString(buffer, text.Content);
    }
#else
    if (withComments)
    {
        result += ToString(buffer, text.CommentBefore);
    }

    result += ToString(buffer, text.Content);

    if (withComments)
    {
        result += ToString(buffer, text.CommentAfter);
    }
#endif

    return result;
}

template <typename CHAR_TYPE>
std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, bool withComments, MultiTextWithComm const & text)
{
    std::basic_string<CHAR_TYPE> result;
    for (auto const & part : text)
    {
        if (result.empty() == false && withComments == false)
            result += " ";
        result += ToString(buffer, withComments, part);
    }
    return result;
}

bool IsEmpty(Mailbox const & mailbox)
{
    return mailbox.Address.Content.LocalPart.Content.second == mailbox.Address.Content.LocalPart.Content.first;
}

template <typename CHAR_TYPE>
class OutputAdapter
{
    std::vector<CHAR_TYPE> buffer_;
    size_t bufferPos_;
public:
    OutputAdapter()
        : bufferPos_(0)
    {
    }

    std::vector<CHAR_TYPE> const & Buffer() const
    {
        return buffer_;
    }

    void operator()(CHAR_TYPE ch, bool isEscapeChar = false)
    {
        if (isEscapeChar)
            return;

        if (bufferPos_ >= buffer_.size())
        {
            buffer_.push_back(ch);
        }
        else
        {
            assert(bufferPos_ < buffer_.size());
            buffer_[bufferPos_] = ch;
        }
        bufferPos_++;
    }

    size_t Pos() const
    {
        return bufferPos_;
    }

    void SetPos(size_t pos)
    {
        bufferPos_ = pos;
    }
};

template <typename INPUT>
class InputAdapter
{
    INPUT & input_;
    using InputResult = decltype(std::declval<INPUT>()());
    std::vector<InputResult> buffer_;
    size_t bufferPos_;
public:
    InputAdapter(INPUT & input)
        : input_(input), bufferPos_(0)
    {
    }

    int operator()()
    {
        if (bufferPos_ >= buffer_.size())
        {
            int ch = input_();
            buffer_.push_back(ch);
        }
        assert(bufferPos_ < buffer_.size());

        return buffer_[bufferPos_++];
    }

    void Back()
    {
        assert(bufferPos_ > 0);
        bufferPos_--;
    }

    bool GetIf(InputResult value)
    {
        if (value == (*this)())
        {
            return true;
        }
        Back();
        return false;
    }

    size_t Pos() const
    {
        return bufferPos_;
    }

    void SetPos(size_t pos)
    {
        bufferPos_ = pos;
    }
};

#define MEMBER_NOTNULL(m, x) (m == nullptr ? nullptr : &m->x)
#define CLEAR_NOTNULL(m) [&]{ if (m) *m = { }; }()
#define WRAP_PARSE(X) [&](auto * elem) { return X(elem); }

template <typename INPUT, typename CHAR_TYPE>
class ParserBase
{
protected:
    InputAdapter<INPUT> input_;
    OutputAdapter<CHAR_TYPE> output_;
public:
    ParserBase(INPUT & input)
        : input_(input)
    {
    }

    auto const & OutputBuffer() const { return output_.Buffer(); }
protected:
    class SavedIOState
    {
        InputAdapter<INPUT> & input_;
        OutputAdapter<CHAR_TYPE> & output_;
        size_t inputPos_;
        size_t outputPos_;
    public:
        SavedIOState(InputAdapter<INPUT> & input, OutputAdapter<CHAR_TYPE> & output)
            : input_(input), output_(output), inputPos_(input.Pos()), outputPos_(output.Pos())
        {
        }

        ~SavedIOState()
        {
            if (inputPos_ != (size_t)-1)
            {
                input_.SetPos(inputPos_);
            }
            if (outputPos_ != (size_t)-1)
            {
                output_.SetPos(outputPos_);
            }
        }

        bool Success()
        {
            inputPos_ = (size_t)-1;
            outputPos_ = (size_t)-1;
            return true;
        }
    };

    SavedIOState Save()
    {
        return SavedIOState(input_, output_);
    }

    template <typename PARSE, typename ELEM = void *>
    bool ParseAtLeastOne(PARSE && parse, std::vector<ELEM> * elems = nullptr)
    {
        bool valid = false;
        for (;;)
        {
            ELEM elem;
            if (parse(&elem))
            {
                if (elems)
                    elems->push_back(elem);
                valid = true;
            }
            else
            {
                break;
            }
        }
        return valid;
    }
    
    template <typename PARSER, typename ELEM>
    bool ParseList(PARSER && parser, std::vector<ELEM> * list)
    {
        // X-list    =   (X *("," X))
        auto ioState(Save());
        for (;;)
        {
            ELEM elem;
            if (parser(&elem) == false)
            {
                CLEAR_NOTNULL(list);
                return false;
            }

            if (list)
                list->push_back(elem);

            if (input_.GetIf(','))
            {
                continue;
            }
            return ioState.Success();
        }
    }

    // Core rules as defined in the Appendix B https://tools.ietf.org/html/rfc5234#appendix-B

    bool ParseVCHAR()
    {
        // VCHAR          =  %x21-7E
        auto ch(this->input_());
        if (ch >= 0x21 && ch < 0x7e)
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    bool ParseWSP(void * = nullptr)
    {
        // WSP            =  SP / HTAB
        auto ch(this->input_());
        if (ch == ' ' || ch == '\t')
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    bool ParseCRLF()
    {
        // CRLF        =  %d13.10
        if (this->input_.GetIf('\r'))
        {
            if (this->input_.GetIf('\n'))
            {
                this->output_('\r');
                this->output_('\n');
                return true;
            }
        }

        return false;
    }

};

// Rules defined in https://tools.ietf.org/html/rfc5322
template <typename INPUT, typename CHAR_TYPE = char>
class ParserRFC5322 : public ParserBase<INPUT, CHAR_TYPE>
{
public:
    ParserRFC5322(INPUT & input)
        : ParserBase(input)
    {
    }
    
    // 3.2.2.  Folding White Space and Comments
    bool ParseFWS()
    {
        // FWS             =   ([*WSP CRLF] 1*WSP) /  obs-FWS
        do
        {
            if (!ParseAtLeastOne(WRAP_PARSE(ParseWSP), (std::vector<void *> *)nullptr))
            {
                return false;
            }
        }
        while (ParseCRLF());

        return true;
    }


    bool ParseCText()
    {
        // ctext           =   %d33-39 /          ; Printable US-ASCII
        //                     %d42-91 /          ;  characters not including
        //                     %d93-126 /         ;  "(", ")", or "\"
        //                     obs-ctext
        auto ch(this->input_());
        if (ch >= 33 && ch <= 39)
            return true;
        if (ch >= 42 && ch <= 91)
            return true;
        if (ch >= 93 && ch <= 126)
            return true;
        this->input_.Back();
        return false;
    }


    bool ParseQuotedPair()
    {
        // quoted-pair     = ("\" (VCHAR / WSP))
        auto ioState(Save());
        if (this->input_.GetIf('\\'))
        {
            this->output_('\\');
            if (ParseVCHAR() || ParseWSP())
            {
                return ioState.Success();
            }
            return true;
        }
        return false;
    }

    bool ParseCContent();

    bool ParseComment()
    {
        // comment         =   "(" *([FWS] ccontent) [FWS] ")"
        auto ioState(Save());
        if (this->input_.GetIf('('))
        {
            this->output_('(');
            do
            {
                ParseFWS();
            } while (ParseCContent());

            ParseFWS();

            if (this->input_.GetIf(')'))
            {
                this->output_(')');
                return ioState.Success();
            }
        }
        return false;
    }

    bool ParseCFWS(SubstringPos * comment)
    {
        // CFWS            =   (1*([FWS] comment) [FWS]) / FWS
        bool valid = false;

        if (comment)
            comment->first = this->output_.Pos();
        for (;;)
        {
            if (ParseFWS())
            {
                valid = true;
            }
            if (ParseComment())
            {
                valid = true;
                continue;
            }
            break;
        }
        if (comment)
            comment->second = this->output_.Pos();

        if (!valid)
        {
            if (comment)
            {
                comment->first = comment->second = this->output_.Pos();
            }
        }

        return valid;
    }


    // 3.2.3.  Atom

    bool ParseAText(void * = nullptr)
    {
        // atext           =   ALPHA / DIGIT /    ; Printable US-ASCII
        //                    "!" / "#" /        ;  characters not including
        //                    "$" / "%" /        ;  specials.  Used for atoms.
        //                    "&" / "'" /
        //                    "*" / "+" /
        //                    "-" / "/" /
        //                    "=" / "?" /
        //                    "^" / "_" /
        //                    "`" / "{" /
        //                    "|" / "}" /
        //                    "~"
        auto isAChar = [](auto ch)
        {
            if (ch >= 'a' && ch <= 'z')
                return true;
            if (ch >= 'A' && ch <= 'Z')
                return true;
            if (ch >= '0' && ch <= '9')
                return true;
            return std::string("!#$%&\'*+-/=?^_`{|}~").find(ch) != std::string::npos;
        };
        auto ch(this->input_());
        if (isAChar(ch))
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    bool ParseAtom(TextWithComm * atom = nullptr)
    {
        // atom            =   [CFWS] 1*atext [CFWS]
        auto ioState(Save());
        ParseCFWS(MEMBER_NOTNULL(atom, CommentBefore));
        size_t c1 = this->output_.Pos();
        if (ParseAtLeastOne(WRAP_PARSE(ParseAText)))
        {
            size_t c2 = this->output_.Pos();
            ParseCFWS(MEMBER_NOTNULL(atom, CommentAfter));

            if (atom)
            {
                atom->Content = std::make_pair(c1, c2);
            }

            return ioState.Success();
        }
        CLEAR_NOTNULL(atom);
        return false;
    }

    bool ParseDotAtomText()
    {
        // dot-atom-text   =   1*atext *("." 1*atext)
        if (ParseAtLeastOne(WRAP_PARSE(ParseAText)))
        {
            auto ioState(Save());
            for (;;)
            {
                if (this->input_.GetIf('.'))
                {
                    this->output_('.');
                    if (false == ParseAtLeastOne(WRAP_PARSE(ParseAText)))
                    {
                        return false;
                    }
                }
                else
                {
                    return ioState.Success();
                }
            }
        }

        return false;
    }

    bool ParseDotAtom(TextWithComm * dotAtom = nullptr)
    {
        // dot-atom        =   [CFWS] dot-atom-text [CFWS]
        auto ioState(Save());
        ParseCFWS(MEMBER_NOTNULL(dotAtom, CommentBefore));
        size_t c1 = this->output_.Pos();
        if (ParseDotAtomText())
        {
            size_t c2 = this->output_.Pos();
            ParseCFWS(MEMBER_NOTNULL(dotAtom, CommentAfter));

            if (dotAtom)
            {
                dotAtom->Content = std::make_pair(c1, c2);
            }

            return ioState.Success();
        }
        CLEAR_NOTNULL(dotAtom);
        return false;
    }

    bool ParseSpecials()
    {
        // specials        =   "(" / ")" /        ; Special characters that do
        //                     "<" / ">" /        ;  not appear in atext
        //                     "[" / "]" /
        //                     ":" / ";" /
        //                     "@" / "\" /
        //                     "," / "." /
        //                     DQUOTE
        auto ch(this->input_());
        if (std::string("()<>[]:;@\\,/\"").find(ch) != std::string::npos)
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    bool ParseQText()
    {
       // qtext           =   %d33 /             ; Printable US-ASCII
       //                     %d35-91 /          ;  characters not including
       //                     %d93-126 /         ;  "\" or the quote character
       //                     obs-qtext
       
        auto ch(this->input_());
        if (ch == 33 || (ch >= 35 && ch <= 91) || (ch >= 93 && ch <= 126))
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    bool ParseQContent()
    {
        // qcontent        =   qtext / quoted-pair

        return ParseQText() || ParseQuotedPair();
    }

    bool ParseQuotedString(TextWithComm * quotedString)
    {
        // quoted-string   =   [CFWS]
        //                     DQUOTE *([FWS] qcontent) [FWS] DQUOTE
        //                     [CFWS]
        auto ioState(Save());
        ParseCFWS(MEMBER_NOTNULL(quotedString, CommentBefore));
        if (this->input_.GetIf('\"'))
        {
            this->output_('\"');
            size_t c1 = this->output_.Pos();
            do
            {
                ParseFWS();
            } while (ParseQContent());
            size_t c2 = this->output_.Pos();

            if (this->input_.GetIf('\"'))
            {
                this->output_('\"');
                ParseCFWS(MEMBER_NOTNULL(quotedString, CommentAfter));

                if (quotedString)
                {
                    quotedString->Content = std::make_pair(c1, c2);
                }

                return ioState.Success();
            }
        }
        CLEAR_NOTNULL(quotedString);
        return false;
    }

    // 3.2.5.  Miscellaneous Tokens

    bool ParseWord(TextWithComm * word)
    {
        // word            =   atom / quoted-string
        return ParseAtom(word) || ParseQuotedString(word);
    }

    bool ParsePhrase(MultiTextWithComm * phrase)
    {
        // phrase          =   1*word / obs-phrase
        return ParseAtLeastOne(WRAP_PARSE(ParseWord), phrase);
    }

    // 3.4.1.  Addr-Spec Specification

    bool ParseLocalPart(TextWithComm * localPart)
    {
        // local-part      =   dot-atom / quoted-string / obs-local-part
        return ParseDotAtom(localPart) || ParseQuotedString(localPart);
    }

    bool ParseDText()
    {
        // dtext           =  %d33-90 /          ; Printable US-ASCII
        //                    %d94-126 /         ;  characters not including
        //                    obs-dtext          ;  "[", "]", or "\"
        auto ch(this->input_());
        if ((ch >= 33 && ch <= 90) || ch >= 94 || ch <= 126)
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    bool ParseDomainLiteral(TextWithComm * domainLiteral)
    {
        // domain-literal  =   [CFWS] "[" *([FWS] dtext) [FWS] "]" [CFWS]
        auto ioState(Save());
        ParseCFWS(MEMBER_NOTNULL(domainLiteral, CommentBefore));
        if (this->input_.GetIf('['))
        {
            size_t c1 = this->output_.Pos();
            this->output_('[');
            do
            {
                ParseFWS();
            } while (ParseDText());

            ParseFWS();
            size_t c2 = this->output_.Pos();

            if (this->input_.GetIf(']'))
            {
                this->output_(']');
                ParseCFWS(MEMBER_NOTNULL(domainLiteral, CommentAfter));
            
                if (domainLiteral)
                {
                    domainLiteral->Content = std::make_pair(c1, c2);
                }

                return ioState.Success();
            }
        }

        CLEAR_NOTNULL(domainLiteral);
        return false;
    }

    bool ParseDomain(TextWithComm * domain)
    {
        // domain          =   dot-atom / domain-literal / obs-domain
        return ParseDotAtom(domain) || ParseDomainLiteral(domain);
    }

    bool ParseAddrSpec(AddrSpec * addrSpec)
    {
        // addr-spec       =   local-part "@" domain
        auto ioState(Save());
        if (ParseLocalPart(MEMBER_NOTNULL(addrSpec, LocalPart)))
        {
            if (this->input_.GetIf('@'))
            {
                this->output_('@');
                if (ParseDomain(MEMBER_NOTNULL(addrSpec, DomainPart)))
                {
                    return ioState.Success();
                }
            }
        }
        CLEAR_NOTNULL(addrSpec);
        return false;
    }

    // 3.4.  Address Specification

    bool ParseAngleAddr(AngleAddr * angleAddr)
    {
        // angle-addr      =   [CFWS] "<" addr-spec ">" [CFWS] /
        //                 obs-angle-addr
        auto ioState(Save());
        ParseCFWS(MEMBER_NOTNULL(angleAddr, CommentBefore));
        if (this->input_.GetIf('<'))
        {
            this->output_('<');
            if (ParseAddrSpec(MEMBER_NOTNULL(angleAddr, Content)))
            {
                if (this->input_.GetIf('>'))
                {
                    this->output_('>');
                    ParseCFWS(MEMBER_NOTNULL(angleAddr, CommentAfter));

                    return ioState.Success();
                }
            }
        }
        CLEAR_NOTNULL(angleAddr);
        return false;
    }

    bool ParseDisplayName(MultiTextWithComm * displayName)
    {
        // display-name    =   phrase
        return ParsePhrase(displayName);
    }

    bool ParseNameAddr(Mailbox * nameAddr)
    {
        // name-addr       =   [display-name] angle-addr
        auto ioState(Save());
        ParseDisplayName(MEMBER_NOTNULL(nameAddr, DisplayName));
        if (ParseAngleAddr(MEMBER_NOTNULL(nameAddr, Address)))
        {
            return ioState.Success();
        }
        CLEAR_NOTNULL(nameAddr);
        return false;
    }

    bool ParseMailbox(Mailbox * mailbox)
    {
        // mailbox         =   name-addr / addr-spec
        return ParseNameAddr(mailbox) || ParseAddrSpec(MEMBER_NOTNULL(mailbox, Address.Content));
    }

    bool ParseMailboxList(MailboxList * mailboxList)
    {
        // mailbox-list    =   (mailbox *("," mailbox)) / obs-mbox-list
        return ParseList(WRAP_PARSE(ParseMailbox), mailboxList);
    }

    bool ParseAddress(Address * address);

    bool ParseAddressList(AddressList * addresses)
    {
        // address-list    =   (address *("," address)) / obs-addr-list
        return ParseList(WRAP_PARSE(ParseAddress), addresses);
    }

    bool ParseGroupList(Group * group)
    {
        // group-list      =   mailbox-list / CFWS / obs-group-list
        return ParseMailboxList(MEMBER_NOTNULL(group, Members)) || ParseCFWS(MEMBER_NOTNULL(group, Comment));
    }

    bool ParseGroup(Group * group)
    {
        // group           =   display-name ":" [group-list] ";" [CFWS]
        auto ioState(Save());
        if (ParseDisplayName(MEMBER_NOTNULL(group, DisplayName)))
        {
            if (this->input_.GetIf(':'))
            {
                this->output_(':');
                ParseGroupList(group);
                if (this->input_.GetIf(';'))
                {
                    this->output_(';');
                    ParseCFWS(MEMBER_NOTNULL(group, Comment));
                    return ioState.Success();
                }
            }
        }

        CLEAR_NOTNULL(group);
        return false;
    }
};

template <typename INPUT, typename CHAR_TYPE>
bool ParserRFC5322<INPUT, CHAR_TYPE>::ParseCContent()
{
    // ccontent        =   ctext / quoted-pair / comment
    return ParseCText() || ParseQuotedPair() || ParseComment();
}

template <typename INPUT, typename CHAR_TYPE>
bool ParserRFC5322<INPUT, CHAR_TYPE>::ParseAddress(Address * address)
{
    // address         =   mailbox / group
    return ParseMailbox(MEMBER_NOTNULL(address, Mailbox)) || ParseGroup(MEMBER_NOTNULL(address, Group));
}

template <typename INPUT, typename CHAR_TYPE>
ParserRFC5322<INPUT, CHAR_TYPE> Make_ParserRFC5322(INPUT && input, CHAR_TYPE charType)
{
    return ParserRFC5322<INPUT, CHAR_TYPE>(input);
}

#include <iostream>

void test_address(std::string const & addr)
{
    std::stringstream some_address(addr);
    std::cout << addr;

    auto parser(Make_ParserRFC5322([&] { return some_address.get(); }, (char)0));

    AddressList addresses;
    if (parser.ParseAddressList(&addresses) && some_address.get() == EOF)
    {
        auto const & outBuffer = parser.OutputBuffer();
        std::cout << " is OK\n";
        std::cout << addresses.size() << " address" << (addresses.size() > 1 ? "es" : "") << " parsed." << std::endl;
        for (auto const & address : addresses)
        {
            auto displayMailBox = [&](auto const & mailbox, auto indent)
            {
                std::cout << indent << "   Mailbox:" << std::endl;
                std::cout << indent << "     Display Name: '" << ToString(outBuffer, true, mailbox.DisplayName) << "'" << std::endl;
                std::cout << indent << "     Adress: " <<std::endl;
                std::cout << indent << "       Local-Part: '" << ToString(outBuffer, false, mailbox.Address.Content.LocalPart) << "'" << std::endl;
                std::cout << indent << "       Domain-Part: '" << ToString(outBuffer, false, mailbox.Address.Content.DomainPart) << "'" << std::endl;
            };

            if (false == IsEmpty(address.Mailbox))
            {
                displayMailBox(address.Mailbox, "");
            }
            else
            {
                std::cout << "   Group:" << std::endl;
                std::cout << "     Display Name: '" << ToString(outBuffer, false, address.Group.DisplayName) << "'" << std::endl;
                std::cout << "     Members:" << std::endl;
                for (auto const & groupAddr : address.Group.Members)
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
    test_address("A@b@c@example.com");

    test_address("simple@example.com");
    test_address("simple(comm1)@(comm2)example.com");
    test_address("very.common@example.com");
    test_address("disposable.style.email.with+symbol@example.com");
    test_address("other.email-with-hyphen@example.com");
    test_address("fully-qualified-domain@example.com");
    test_address("user.name+tag+sorting@example.com");
    test_address("troll@bitch.com, arobar     d <sigma@addr.net>, sir john snow <user.name+tag+sorting@example.com(comment)>");
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
