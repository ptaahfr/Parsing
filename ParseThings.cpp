// test parsing code for email adresses based on RFC 5322 & 5234
// (c) Ptaah

#include <cstdint>
#include <type_traits>
#include <vector>
#include <exception>
#include <sstream>
#include <algorithm>
#include <cassert>

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

    void operator()(CHAR_TYPE ch, bool isSyntaxChar = false)
    {
        if (isSyntaxChar)
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

    size_t GetPos() const
    {
        return bufferPos_;
    }

    void SetPos(size_t pos)
    {
        bufferPos_ = pos;
    }

    template <typename OUTPUT>
    void FlushTo(OUTPUT & output)
    {
        for (auto ch : buffer_)
        {
            output(ch);
        }
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

    size_t GetPos() const
    {
        return bufferPos_;
    }

    void SetPos(size_t pos)
    {
        bufferPos_ = pos;
    }
};

template <typename INPUT>
InputAdapter<INPUT> Make_InputAdapter(INPUT & input)
{
    return InputAdapter<INPUT>(input);
}

template <typename PARENT>
class IOState
{
    PARENT & parent_;
    size_t pos_;
public:
    IOState(PARENT & parent)
        : parent_(parent), pos_(parent_.GetPos())
    {
    }

    ~IOState()
    {
        if (pos_ != (size_t)-1)
        {
            parent_.SetPos(pos_);
        }
    }

    bool Success()
    {
        pos_ = -1;
        return true;
    }
};

template <typename PARENT>
IOState<PARENT> Make_IOState(PARENT & parent)
{
    return IOState<PARENT>(parent);
}

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
bool ParseWSP(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
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

template <typename PARSE, typename CHAR_TYPE, typename INPUT>
bool ParseAtLeastOne(PARSE & parse, OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    bool valid = false;
    while (parse(output, input))
        valid = true;
    return valid;
}

#define WRAP_PARSE(X) [](auto & output, auto & input) { return X(output, input); }

template <typename CHAR_TYPE, typename INPUT>
bool ParseFWS(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // FWS             =   ([*WSP CRLF] 1*WSP) /  obs-FWS
    do
    {
        if (!ParseAtLeastOne(WRAP_PARSE(ParseWSP), output, input))
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
    //                        %d42-91 /          ;  characters not including
    //                        %d93-126 /         ;  "(", ")", or "\"
    //                        obs-ctext
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
    if (input.GetIf('\\'))
    {
        output('\\', true);
        output(input());
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
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    if (input.GetIf('('))
    {
        output('(', true);
        do
        {
            ParseFWS(output, input);
        } while (ParseCContent(output, input));

        ParseFWS(output, input);

        if (input.GetIf(')'))
        {
            output(')', true);
            return outputState.Success() && inputState.Success();
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
bool ParseCFWS(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // CFWS            =   (1*([FWS] comment) [FWS]) / FWS
    bool valid = false;
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
    return valid;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
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
bool ParseAtom(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // atom            =   [CFWS] 1*atext [CFWS]
    bool valid = false;
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    ParseCFWS(output, input);
    if (ParseAtLeastOne(WRAP_PARSE(ParseAText), output, input))
    {
        ParseCFWS(output, input);
        return outputState.Success() && inputState.Success();
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDotAtomText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // dot-atom-text   =   1*atext *("." 1*atext)
    if (ParseAtLeastOne(WRAP_PARSE(ParseAText), output, input))
    {
        auto inputState(Make_IOState(input));
        auto outputState(Make_IOState(output));
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
                return outputState.Success() && inputState.Success();
            }
        }
    }

    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDotAtom(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // dot-atom        =   [CFWS] dot-atom-text [CFWS]
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    ParseCFWS(output, input);
    if (ParseDotAtomText(output, input))
    {
        ParseCFWS(output, input);
        return outputState.Success() && inputState.Success();
    }
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
bool ParseQuotedString(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // quoted-string   =   [CFWS]
    //                     DQUOTE *([FWS] qcontent) [FWS] DQUOTE
    //                     [CFWS]
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    ParseCFWS(output, input);
    if (input.GetIf('\"'))
    {
        output('\"', true);
        do
        {
            ParseFWS(output, input);
        } while (ParseQContent(output, input));

        ParseFWS(output, input);
        if (input.GetIf('\"'))
        {
            output('\"', true);
            ParseCFWS(output, input);
            return outputState.Success() && inputState.Success();
        }
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseWord(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // word            =   atom / quoted-string
    return ParseAtom(output, input) || ParseQuotedString(output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParsePhrase(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // phrase          =   1*word / obs-phrase
    return ParseAtLeastOne(WRAP_PARSE(ParseWord), output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseLocalPart(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // local-part      =   dot-atom / quoted-string / obs-local-part
    return ParseDotAtom(output, input) || ParseQuotedString(output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDText(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // dtext           =  %d33-90 /          ; Printable US-ASCII
    //                    %d94-126 /         ;  characters not including
    //                    obs-dtext          ;  "[", "]", or "\"
    auto ch(input());
    if (ch >= 33 && ch <= 90 || ch >= 94 || ch <= 126)
    {
        output(ch);
        return true;
    }
    input.Back();
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDomainLiteral(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // domain-literal  =   [CFWS] "[" *([FWS] dtext) [FWS] "]" [CFWS]
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    ParseCFWS(output, input);
    if (input.GetIf('['))
    {
        output('[', true);
        do
        {
            ParseFWS(output, input);
        } while (ParseDText(output, input));

        ParseFWS(output, input);

        if (input.GetIf(']'))
        {
            output(']', true);
            ParseCFWS(output, input);
            return outputState.Success() && inputState.Success();
        }
    }

    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDomain(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // domain          =   dot-atom / domain-literal / obs-domain
    return ParseDotAtom(output, input) || ParseDomainLiteral(output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddrSpec(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // addr-spec       =   local-part "@" domain
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    if (ParseLocalPart(output, input))
    {
        if (input.GetIf('@'))
        {
            output('@');
            if (ParseDomain(output, input))
            {
                return outputState.Success() && inputState.Success();
            }
        }
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAngleAddr(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // angle-addr      =   [CFWS] "<" addr-spec ">" [CFWS] /
    //                 obs-angle-addr
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    ParseCFWS(output, input);
    if (input.GetIf('<'))
    {
        output('<');
        if (ParseAddrSpec(output, input))
        {
            if (input.GetIf('>'))
            {
                output('>');
                ParseCFWS(output, input);
                return outputState.Success() && inputState.Success();
            }
        }
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseDisplayName(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // display-name    =   phrase
    return ParsePhrase(output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseNameAddr(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // name-addr       =   [display-name] angle-addr
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    ParseDisplayName(output, input);
    if (ParseAngleAddr(output, input))
    {
        return outputState.Success() && inputState.Success();
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseMailbox(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // mailbox         =   name-addr / addr-spec
    return ParseNameAddr(output, input) || ParseAddrSpec(output, input);
}

template <typename PARSER, typename CHAR_TYPE, typename INPUT>
bool ParseList(PARSER & parser, OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // X-list    =   (X *("," X))
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    for (;;)
    {
        if (parser(output, input) == false)
            return false;
        if (input.GetIf(','))
        {
            continue;
        }
        return outputState.Success() && inputState.Success();
    }
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseMailboxList(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // mailbox-list    =   (mailbox *("," mailbox)) / obs-mbox-list
    return ParseList(WRAP_PARSE(ParseMailbox), output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddress(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input);

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddressList(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // address-list    =   (address *("," address)) / obs-addr-list
    return ParseList(WRAP_PARSE(ParseAddress), output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseGroupList(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // group-list      =   mailbox-list / CFWS / obs-group-list
    return ParseMailboxList(output, input) || ParseCFWS(output, input);
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseGroup(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // group           =   display-name ":" [group-list] ";" [CFWS]
    auto inputState(Make_IOState(input));
    auto outputState(Make_IOState(output));
    if (ParseDisplayName(output, input))
    {
        if (input.GetIf(':'))
        {
            output(':');
            if (ParseGroupList(output, input))
            {
                if (input.GetIf(';'))
                {
                    output(';');
                    ParseCFWS(output, input);
                    return outputState.Success() && inputState.Success();
                }
            }
        }
    }
    return false;
}

template <typename CHAR_TYPE, typename INPUT>
bool ParseAddress(OutputAdapter<CHAR_TYPE> & output, InputAdapter<INPUT> & input)
{
    // address         =   mailbox / group
    return ParseMailbox(output, input) || ParseGroup(output, input);
}

#include <iostream>

void test_address(std::string const & addr)
{
    std::stringstream some_address(addr);
    std::cout << addr;
    OutputAdapter<char> output;
    if (ParseAddressList(output, Make_InputAdapter([&] { return some_address.get(); })) && some_address.get() == EOF)
    {
        std::cout << " is OK\n";
    }
    else
    {
        std::cout << " is NOK\n";
    }
}

int main()
{
    test_address("A@b@c@example.com");

    test_address("simple@example.com");
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

    test_address("Abc.example.com");
    test_address("a\"b(c)d,e:f;g<h>i[j\\k]l@example.com");
    test_address("just\"not\"right@example.com");
    test_address("this is\"not\\allowed@example.com");
    test_address("this\\ still\\\"not\\\\allowed@example.com");
    test_address("1234567890123456789012345678901234567890123456789012345678901234+x@example.com");

    return 0;
}