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
    if (withComments)
    {
        result += ToString(buffer, text.CommentBefore);
    }

    result += ToString(buffer, text.Content);

    if (withComments)
    {
        result += ToString(buffer, text.CommentAfter);
    }

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

template <typename INPUT>
InputAdapter<INPUT> Make_InputAdapter(INPUT && input)
{
    return InputAdapter<INPUT>(input);
}

template <typename INPUT, typename OUTPUT>
class SavedIOState
{
    INPUT & input_;
    OUTPUT & output_;
    size_t inputPos_;
    size_t outputPos_;
public:
    SavedIOState(INPUT & input, OUTPUT & output)
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

template <typename INPUT, typename OUTPUT>
SavedIOState<INPUT, OUTPUT> SaveIOState(INPUT & input, OUTPUT & output)
{
    return SavedIOState<INPUT, OUTPUT>(input, output);
}

#define MEMBER_NOTNULL(m, x) (m == nullptr ? nullptr : &m->x)
#define CLEAR_NOTNULL(m) { if (m) *m = { }; }

template <typename CHAR_TYPE, typename INPUT>
bool ParseVCHAR(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // VCHAR          =  %x21-7E
    auto ch(input());
    if (ch >= 0x21 && ch < 0x7e)
    {
        output(ch);
        return true;
    }
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseWSP(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, void * = nullptr)
{
    // WSP            =  SP / HTAB
    auto ch(input());
    if (ch == ' ' || ch == '\t')
    {
        output(ch);
        return true;
    }
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseCRLF(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // CRLF        =  %d13.10
    if (input.GetIf('\r'))
    {
        if (input.GetIf('\n'))
        {
            output('\r');
            output('\n');
            return true;
        }
    }

    return false;
}

template <typename PARSE, typename CHAR_TYPE, typename INPUT, typename ELEM = void *>
bool ParseAtLeastOne(PARSE && parse, OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, std::vector<ELEM> * elems = nullptr)
{
    bool valid = false;
    for (;;)
    {
        ELEM elem;
        if (parse(output, input, &elem))
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

#define WRAP_PARSE(X) [](auto & output, auto & input, auto * elem) { return X(output, input, elem); }

template <typename CHAR_TYPE, typename INPUT>
bool ParseFWS(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // FWS             =   ([*WSP CRLF] 1*WSP) /  obs-FWS
    do
    {
        if (!ParseAtLeastOne(WRAP_PARSE(ParseWSP), output, input, (std::vector<void *> *)nullptr))
        {
            return false;
        }
    }
    while (ParseCRLF(output, input));

    return true;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseCText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // ctext           =   %d33-39 /          ; Printable US-ASCII
    //                     %d42-91 /          ;  characters not including
    //                     %d93-126 /         ;  "(", ")", or "\"
    //                     obs-ctext
    auto ch(input());
    if (ch >= 33 && ch <= 39)
        return true;
    if (ch >= 42 && ch <= 91)
        return true;
    if (ch >= 93 && ch <= 126)
        return true;
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseQuotedPair(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // quoted-pair     = ("\" (VCHAR / WSP))
    auto ioState(SaveIOState(input, output));
    if (input.GetIf('\\'))
    {
        output('\\');
        if (ParseVCHAR(output, input) || ParseWSP(output, input))
        {
            return ioState.Success();
        }
        return true;
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseCContent(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input);

template <typename CHAR_TYPE, typename INPUT>
bool ParseComment(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // comment         =   "(" *([FWS] ccontent) [FWS] ")"
    auto ioState(SaveIOState(input, output));
    if (input.GetIf('('))
    {
        output('(');
        do
        {
            ParseFWS(output, input);
        } while (ParseCContent(output, input));

        ParseFWS(output, input);

        if (input.GetIf(')'))
        {
            output(')');
            return ioState.Success();
        }
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseCContent(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // ccontent        =   ctext / quoted-pair / comment
    return ParseCText(output, input) || ParseQuotedPair(output, input) || ParseComment(output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseCFWS(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, SubstringPos * comment)
{
    // CFWS            =   (1*([FWS] comment) [FWS]) / FWS
    bool valid = false;
    if (comment)
        comment->first = output.Pos();
    for (;;)
    {
        if (ParseFWS(output, input))
        {
            valid = true;
        }
        if (ParseComment(output, input))
        {
            valid = true;
            continue;
        }
        break;
    }
    if (comment)
        comment->second = output.Pos();
    if (!valid)
        CLEAR_NOTNULL(comment);
    return valid;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, void * = nullptr)
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
    auto ch(input());
    if (isAChar(ch))
    {
        output(ch);
        return true;
    }
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAtom(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, TextWithComm * atom = nullptr)
{
    // atom            =   [CFWS] 1*atext [CFWS]
    auto ioState(SaveIOState(input, output));
    ParseCFWS(output, input, MEMBER_NOTNULL(atom, CommentBefore));
    size_t c1 = output.Pos();
    if (ParseAtLeastOne(WRAP_PARSE(ParseAText), output, input))
    {
        size_t c2 = output.Pos();
        ParseCFWS(output, input, MEMBER_NOTNULL(atom, CommentAfter));

        if (atom)
        {
            atom->Content = std::make_pair(c1, c2);
        }

        return ioState.Success();
    }
    CLEAR_NOTNULL(atom);
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDotAtomText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // dot-atom-text   =   1*atext *("." 1*atext)
    if (ParseAtLeastOne(WRAP_PARSE(ParseAText), output, input))
    {
        auto ioState(SaveIOState(input, output));
        for (;;)
        {
            if (input.GetIf('.'))
            {
                output('.');
                if (false == ParseAtLeastOne(WRAP_PARSE(ParseAText), output, input))
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

template <typename CHAR_TYPE, typename INPUT>
bool ParseDotAtom(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, TextWithComm * dotAtom = nullptr)
{
    // dot-atom        =   [CFWS] dot-atom-text [CFWS]
    auto ioState(SaveIOState(input, output));
    ParseCFWS(output, input, MEMBER_NOTNULL(dotAtom, CommentBefore));
    size_t c1 = output.Pos();
    if (ParseDotAtomText(output, input))
    {
        size_t c2 = output.Pos();
        ParseCFWS(output, input, MEMBER_NOTNULL(dotAtom, CommentAfter));

        if (dotAtom)
        {
            dotAtom->Content = std::make_pair(c1, c2);
        }

        return ioState.Success();
    }
    CLEAR_NOTNULL(dotAtom);
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseSpecials(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // specials        =   "(" / ")" /        ; Special characters that do
    //                     "<" / ">" /        ;  not appear in atext
    //                     "[" / "]" /
    //                     ":" / ";" /
    //                     "@" / "\" /
    //                     "," / "." /
    //                     DQUOTE
    auto ch(input());
    if (std::string("()<>[]:;@\\,/\"").find(ch) != std::string::npos)
    {
        output(ch);
        return true;
    }
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseQText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
   // qtext           =   %d33 /             ; Printable US-ASCII
   //                     %d35-91 /          ;  characters not including
   //                     %d93-126 /         ;  "\" or the quote character
   //                     obs-qtext
       
    auto ch(input());
    if (ch == 33 || (ch >= 35 && ch <= 91) || (ch >= 93 && ch <= 126))
    {
        output(ch);
        return true;
    }
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseQContent(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // qcontent        =   qtext / quoted-pair

    return ParseQText(output, input) || ParseQuotedPair(output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseQuotedString(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, TextWithComm * quotedString)
{
    // quoted-string   =   [CFWS]
    //                     DQUOTE *([FWS] qcontent) [FWS] DQUOTE
    //                     [CFWS]
    auto ioState(SaveIOState(input, output));
    ParseCFWS(output, input, MEMBER_NOTNULL(quotedString, CommentBefore));
    if (input.GetIf('\"'))
    {
        output('\"');
        size_t c1 = output.Pos();
        do
        {
            ParseFWS(output, input);
        } while (ParseQContent(output, input));
        size_t c2 = output.Pos();

        if (input.GetIf('\"'))
        {
            output('\"');
            ParseCFWS(output, input, MEMBER_NOTNULL(quotedString, CommentAfter));

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

template <typename CHAR_TYPE, typename INPUT>
bool ParseWord(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, TextWithComm * word)
{
    // word            =   atom / quoted-string
    return ParseAtom(output, input, word) || ParseQuotedString(output, input, word);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParsePhrase(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, MultiTextWithComm * phrase)
{
    // phrase          =   1*word / obs-phrase
    return ParseAtLeastOne(WRAP_PARSE(ParseWord), output, input, phrase);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseLocalPart(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, TextWithComm * localPart)
{
    // local-part      =   dot-atom / quoted-string / obs-local-part
    return ParseDotAtom(output, input, localPart) || ParseQuotedString(output, input, localPart);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // dtext           =  %d33-90 /          ; Printable US-ASCII
    //                    %d94-126 /         ;  characters not including
    //                    obs-dtext          ;  "[", "]", or "\"
    auto ch(input());
    if ((ch >= 33 && ch <= 90) || ch >= 94 || ch <= 126)
    {
        output(ch);
        return true;
    }
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDomainLiteral(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, TextWithComm * domainLiteral)
{
    // domain-literal  =   [CFWS] "[" *([FWS] dtext) [FWS] "]" [CFWS]
    auto ioState(SaveIOState(input, output));
    ParseCFWS(output, input, MEMBER_NOTNULL(domainLiteral, CommentBefore));
    if (input.GetIf('['))
    {
        size_t c1 = output.Pos();
        output('[');
        do
        {
            ParseFWS(output, input);
        } while (ParseDText(output, input));

        ParseFWS(output, input);
        size_t c2 = output.Pos();

        if (input.GetIf(']'))
        {
            output(']');
            ParseCFWS(output, input, MEMBER_NOTNULL(domainLiteral, CommentAfter));
            
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

template <typename CHAR_TYPE, typename INPUT>
bool ParseDomain(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, TextWithComm * domain)
{
    // domain          =   dot-atom / domain-literal / obs-domain
    return ParseDotAtom(output, input, domain) || ParseDomainLiteral(output, input, domain);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddrSpec(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, AddrSpec * addrSpec)
{
    // addr-spec       =   local-part "@" domain
    auto ioState(SaveIOState(input, output));
    if (ParseLocalPart(output, input, MEMBER_NOTNULL(addrSpec, LocalPart)))
    {
        if (input.GetIf('@'))
        {
            output('@');
            if (ParseDomain(output, input, MEMBER_NOTNULL(addrSpec, DomainPart)))
            {
                return ioState.Success();
            }
        }
    }
    CLEAR_NOTNULL(addrSpec);
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAngleAddr(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, AngleAddr * angleAddr)
{
    // angle-addr      =   [CFWS] "<" addr-spec ">" [CFWS] /
    //                 obs-angle-addr
    auto ioState(SaveIOState(input, output));
    ParseCFWS(output, input, MEMBER_NOTNULL(angleAddr, CommentBefore));
    if (input.GetIf('<'))
    {
        output('<');
        if (ParseAddrSpec(output, input, MEMBER_NOTNULL(angleAddr, Content)))
        {
            if (input.GetIf('>'))
            {
                output('>');
                ParseCFWS(output, input, MEMBER_NOTNULL(angleAddr, CommentAfter));

                return ioState.Success();
            }
        }
    }
    CLEAR_NOTNULL(angleAddr);
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDisplayName(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, MultiTextWithComm * displayName)
{
    // display-name    =   phrase
    return ParsePhrase(output, input, displayName);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseNameAddr(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, Mailbox * nameAddr)
{
    // name-addr       =   [display-name] angle-addr
    auto ioState(SaveIOState(input, output));
    ParseDisplayName(output, input, MEMBER_NOTNULL(nameAddr, DisplayName));
    if (ParseAngleAddr(output, input, MEMBER_NOTNULL(nameAddr, Address)))
    {
        return ioState.Success();
    }
    CLEAR_NOTNULL(nameAddr);
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseMailbox(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, Mailbox * mailbox)
{
    // mailbox         =   name-addr / addr-spec
    return ParseNameAddr(output, input, mailbox) || ParseAddrSpec(output, input, MEMBER_NOTNULL(mailbox, Address.Content));
}

template <typename PARSER, typename CHAR_TYPE, typename INPUT, typename ELEM>
bool ParseList(PARSER && parser, OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, std::vector<ELEM> * list)
{
    // X-list    =   (X *("," X))
    auto ioState(SaveIOState(input, output));
    for (;;)
    {
        ELEM elem;
        if (parser(output, input, &elem) == false)
        {
            CLEAR_NOTNULL(list);
            return false;
        }

        if (list)
            list->push_back(elem);

        if (input.GetIf(','))
        {
            continue;
        }
        return ioState.Success();
    }
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseMailboxList(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, MailboxList * mailboxList)
{
    // mailbox-list    =   (mailbox *("," mailbox)) / obs-mbox-list
    return ParseList(WRAP_PARSE(ParseMailbox), output, input, mailboxList);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddress(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input);

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddressList(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, AddressList * addresses)
{
    // address-list    =   (address *("," address)) / obs-addr-list
    return ParseList(WRAP_PARSE(ParseAddress), output, input, addresses);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseGroupList(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, Group * group)
{
    // group-list      =   mailbox-list / CFWS / obs-group-list
    return ParseMailboxList(output, input, MEMBER_NOTNULL(group, Members)) || ParseCFWS(output, input, MEMBER_NOTNULL(group, Comment));
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseGroup(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, Group * group)
{
    // group           =   display-name ":" [group-list] ";" [CFWS]
    auto ioState(SaveIOState(input, output));
    if (ParseDisplayName(output, input, MEMBER_NOTNULL(group, DisplayName)))
    {
        if (input.GetIf(':'))
        {
            output(':');
            ParseGroupList(output, input, group);
            if (input.GetIf(';'))
            {
                output(';');
                ParseCFWS(output, input, MEMBER_NOTNULL(group, Comment));
                return ioState.Success();
            }
        }
    }

    CLEAR_NOTNULL(group);
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddress(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input, Address * address)
{
    // address         =   mailbox / group
    return ParseMailbox(output, input, MEMBER_NOTNULL(address, Mailbox)) || ParseGroup(output, input, MEMBER_NOTNULL(address, Group));
}

#include <iostream>

void test_address(std::string const & addr)
{
    std::stringstream some_address(addr);
    std::cout << addr;
    OutputAdapter<char> output;
    auto input(Make_InputAdapter([&] { return some_address.get(); }));

    AddressList addresses;
    if (ParseAddressList(output, input, &addresses) && some_address.get() == EOF)
    {
        std::cout << " is OK\n";
        std::cout << addresses.size() << " address" << (addresses.size() > 1 ? "es" : "") << " parsed." << std::endl;
        for (auto const & address : addresses)
        {
            auto displayMailBox = [&](auto const & mailbox, auto indent)
            {
                std::cout << indent << "   Mailbox:" << std::endl;
                std::cout << indent << "     Display Name: '" << ToString(output.Buffer(), true, mailbox.DisplayName) << "'" << std::endl;
                std::cout << indent << "     Adress: " <<std::endl;
                std::cout << indent << "       Local-Part: '" << ToString(output.Buffer(), false, mailbox.Address.Content.LocalPart) << "'" << std::endl;
                std::cout << indent << "       Domain-Part: '" << ToString(output.Buffer(), false, mailbox.Address.Content.DomainPart) << "'" << std::endl;
            };

            if (false == IsEmpty(address.Mailbox))
            {
                displayMailBox(address.Mailbox, "");
            }
            else
            {
                std::cout << "   Group:" << std::endl;
                std::cout << "     Display Name: '" << ToString(output.Buffer(), false, address.Group.DisplayName) << "'" << std::endl;
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
