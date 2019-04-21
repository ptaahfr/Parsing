// test parsing code for email adresses based on RFC 5322 & 5234
// (c) Ptaah

#include <cstdint>
#include <type_traits>
#include <tuple>
#include <utility>
#include <vector>
#include <exception>
#include <sstream>
#include <algorithm>
#include <cassert>

#ifdef _MSC_VER
#pragma warning(disable: 4503)
#endif

using SubstringPos = std::pair<size_t, size_t>;

bool IsEmpty(SubstringPos sub)
{
    return sub.second <= sub.first;
}

template <typename ELEM>
bool IsEmpty(std::vector<ELEM> arr);

template <size_t OFFSET, typename... ARGS>
bool IsEmpty(std::tuple<ARGS...> tuple);

template <typename... ARGS>
bool IsEmpty(std::tuple<ARGS...> tuple)
{
    return IsEmpty<0>(tuple);
}

template <size_t OFFSET, typename... ARGS>
bool IsEmpty(std::tuple<ARGS...> tuple)
{
    enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(ARGS) - 1) };
    if (false == IsEmpty(std::get<OFFSET>(tuple)))
        return false;
    if (NEXT_OFFSET > OFFSET)
        return IsEmpty<NEXT_OFFSET>(tuple);
    return true;
}


template <typename ELEM>
bool IsEmpty(std::vector<ELEM> arr)
{
    for (auto const & elem : arr)
    {
        if (false == IsEmpty(elem))
            return false;
    }
    return true;
}

template <typename ELEM>
ELEM ElemTypeOrNull(std::vector<ELEM> const *);

nullptr_t ElemTypeOrNull(nullptr_t = nullptr);

template <size_t S1, size_t S2, typename PTR, std::enable_if_t<S1 == S2, void *> = nullptr>
PTR NonNullIfEqual(PTR ptr)
{
    return ptr;
}

template <size_t S1, size_t S2, typename PTR, std::enable_if_t<S1 != S2, void *> = nullptr>
nullptr_t NonNullIfEqual(PTR ptr)
{
    return nullptr;
}

template <typename ELEM>
ELEM * PtrOrNull(ELEM & e)
{
    return &e;
}

nullptr_t PtrOrNull(nullptr_t = nullptr)
{
    return nullptr;
}

using TextWithComm = std::tuple<SubstringPos, SubstringPos, SubstringPos>; // CommentBefore, Content, CommentAfter
enum TextWithCommFields
{
    TextWithCommFields_CommentBefore,
    TextWithCommFields_Content,
    TextWithCommFields_CommentAfter
};

using MultiTextWithComm = std::vector<TextWithComm>;

using AddrSpec = std::tuple<TextWithComm, TextWithComm>; // LocalPart, DomainPart
enum AddrSpecFields
{
    AddrSpecFields_LocalPart,
    AddrSpecFields_DomainPart,
};

using AngleAddr = std::tuple<SubstringPos, AddrSpec, SubstringPos>; // CommentBefore, Content, CommentAfter
enum AngleAddrFields
{
    AngleAddrFields_CommentBefore,
    AngleAddrFields_Content,
    AngleAddrFields_CommentAfter
};

using Mailbox = std::tuple<MultiTextWithComm, AngleAddr>; // DisplayName, Address
enum MailboxFields
{
    MailboxFields_DisplayName,
    MailboxFields_Address,
};

using MailboxList = std::vector<Mailbox>;

using GroupList = std::tuple<MailboxList, SubstringPos>; // Mailboxes, Comment
enum GroupListFields
{
    GroupListFields_Mailboxes,
    GroupListFields_Comment
};

using Group = std::tuple<MultiTextWithComm, GroupList, SubstringPos>; // DisplayName, GroupList, Comment
enum GroupFields
{
    GroupFields_DisplayName,
    GroupFields_GroupList,
    GroupFields_Comment
};

using Address = std::tuple<Mailbox, Group>;
enum AddressFields
{
    AddressFields_Mailbox,
    AddressFields_Group
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
        result += ToString(buffer, SubstringPos(std::get<TextWithCommFields_CommentBefore>(text).first, std::get<TextWithCommFields_CommentAfter>(text).second));
    }
    else
    {
        result += ToString(buffer, std::get<TextWithCommFields_Content>(text));
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
    INPUT input_;
    using InputResult = decltype(std::declval<INPUT>()());
    std::vector<InputResult> buffer_;
    size_t bufferPos_;
public:
    InputAdapter(INPUT input)
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

template <size_t INDEX, typename... ARGS, std::enable_if_t<(INDEX < sizeof...(ARGS)), void *> = nullptr>
auto FieldNotNull(std::tuple<ARGS...> * tu) -> decltype(&std::get<INDEX>(*tu))
{
    if (tu != nullptr)
    {
        return &std::get<INDEX>(*tu);
    }
    return nullptr;
}

template <size_t INDEX, typename... ARGS, std::enable_if_t<(INDEX >= sizeof...(ARGS)), void *> = nullptr>
nullptr_t FieldNotNull(std::tuple<ARGS...> * tu)
{
    return nullptr;
}

template <size_t INDEX>
nullptr_t FieldNotNull(nullptr_t = nullptr)
{
    return nullptr;
}

template <typename TYPE>
void ClearNotNull(TYPE * t)
{
    if (t != nullptr)
    {
        *t = {};
    }
}

void ClearNotNull(nullptr_t = nullptr)
{
}

template <typename ELEM>
void PushBackIfNotNull(std::vector<ELEM> * elems, ELEM const & elem)
{
    if (elems != nullptr)
    {
        elems->push_back(elem);
    }
}

template <typename ELEM>
void PushBackIfNotNull(nullptr_t, ELEM const &)
{
}

template <typename ELEM>
void ResizeIfNotNull(std::vector<ELEM> * elems, size_t newSize)
{
    if (elems != nullptr)
    {
        elems->resize(newSize);
    }
}

void ResizeIfNotNull(nullptr_t, size_t newSize)
{
}

template <typename ELEM>
size_t GetSizeIfNotNull(std::vector<ELEM> const * elems)
{
    if (elems != nullptr)
    {
        return elems->size();
    }
    return 0;
}

size_t GetSizeIfNotNull(nullptr_t)
{
    return 0;
}

#define FIELD_NOTNULL(m, x) FieldNotNull<x>(m)
#define CLEAR_NOTNULL(m) ClearNotNull(m)

#define THIS_PARSE(X) [&] (auto elem) { return this->X(elem); }
#define THIS_PARSE_ARGS(X, ...) [&] (auto elem) { return this->X(elem, __VA_ARGS__); }

template <typename INPUT, typename CHAR_TYPE>
class ParserBase
{
protected:
    InputAdapter<INPUT> input_;
    OutputAdapter<CHAR_TYPE> output_;
public:
    ParserBase(INPUT input)
        : input_(input)
    {
    }

    auto const & OutputBuffer() const { return output_.Buffer(); }
    bool Ended() { return input_() == EOF; }
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

    template <typename ELEMS_PTR, typename PARSER>
    bool ParseRepeat(ELEMS_PTR elems, PARSER parser, size_t minCount = 0, size_t maxCount = SIZE_MAX)
    {
        size_t count = 0;
        size_t prevSize = GetSizeIfNotNull(elems);

        auto ioState(this->Save());

        for (; count < maxCount; ++count)
        {
            decltype(ElemTypeOrNull(elems)) elem = {};
            if (parser(PtrOrNull(elem)))
            {
                PushBackIfNotNull(elems, elem);
            }
            else
            {
                break;
            }
        }

        if (count < minCount)
        {
            ResizeIfNotNull(elems, prevSize);
            return false;
        }

        return ioState.Success();
    }

    template <typename ELEM_PTR, typename PARSER>
    bool ParseOptional(ELEM_PTR elem, PARSER parser)
    {
        return parser(elem) || true;
    }

    template <typename PREDICATE>
    bool ParseChar(PREDICATE pred)
    {
        auto ch(this->input_());
        if (pred(ch))
        {
            this->output_(ch);
            return true;
        }
        this->input_.Back();
        return false;
    }

    template <typename CHAR_TYPE>
    bool ParseCharExact(CHAR_TYPE ch, bool escape)
    {
        if (input_.GetIf(ch))
        {
            this->output_(ch, escape);
            return true;
        }
        return false;
    }
    
    template <typename CHAR_TYPE>
    bool ParseCharExact(nullptr_t, CHAR_TYPE ch, bool escape)
    {
        return ParseCharExact(ch, escape);
    }

private:
    template <size_t OFFSET, typename AST_PTR, typename PARSER>
    bool ParseSequenceItem(AST_PTR ast, PARSER parser)
    {
        return parser(FIELD_NOTNULL(ast, OFFSET));
    }
    
    template <size_t OFFSET, typename AST_PTR, typename PARSER, typename... OTHERS>
    bool ParseSequenceItem(AST_PTR ast, PARSER parser, OTHERS... others)
    {
        if (parser(FIELD_NOTNULL(ast, OFFSET)))
        {
            if (ParseSequenceItem<OFFSET + 1>(ast, others...))
            {
                return true;
            }
        }
        return false;
    }

    template <size_t OFFSET, size_t FIELD_INDEX, typename AST_PTR, typename PARSER>
    bool ParseSequenceOneFieldItem(AST_PTR ast, PARSER parser)
    {
        return parser(NonNullIfEqual<OFFSET, FIELD_INDEX>(ast));
    }
    
    template <size_t OFFSET, size_t FIELD_INDEX, typename AST_PTR, typename PARSER, typename... OTHERS>
    bool ParseSequenceOneFieldItem(AST_PTR ast, PARSER parser, OTHERS... others)
    {
        if (parser(NonNullIfEqual<OFFSET, FIELD_INDEX>(ast)))
        {
            if (ParseSequenceOneFieldItem<OFFSET + 1, FIELD_INDEX>(ast, others...))
            {
                return true;
            }
        }
        return false;
    }

    template <size_t OFFSET, size_t FIELD_INDEX, typename AST_PTR, typename PARSER>
    bool ParseSequenceFieldsItem(AST_PTR ast, std::index_sequence<FIELD_INDEX>, PARSER parser)
    {
        return parser(FieldNotNull<FIELD_INDEX>(ast));
    }
    
    template <size_t OFFSET, size_t FIELD_INDEX, size_t... OTHER_FIELD_INDICES, typename AST_PTR, typename PARSER, typename... OTHERS>
    bool ParseSequenceFieldsItem(AST_PTR ast, std::index_sequence<FIELD_INDEX, OTHER_FIELD_INDICES...>, PARSER parser, OTHERS... others)
    {
        if (parser(FieldNotNull<FIELD_INDEX>(ast)))
        {
            if (ParseSequenceFieldsItem<OFFSET + 1>(ast, std::index_sequence<OTHER_FIELD_INDICES...>(), others...))
            {
                return true;
            }
        }
        return false;
    }
protected:
    template <typename AST_PTR = nullptr_t, typename... PARSERS>
    bool ParseSequence(AST_PTR ast, PARSERS... parsers)
    {
        auto ioState(this->Save());
        if (ParseSequenceItem<0>(ast, parsers...))
        {
            return ioState.Success();
        }
        CLEAR_NOTNULL(ast);
        return false;
    }

    template <size_t FIELD_INDEX, typename AST_PTR = nullptr_t, typename... PARSERS>
    bool ParseSequenceOneField(AST_PTR ast, PARSERS... parsers)
    {
        auto ioState(this->Save());
        if (ParseSequenceOneFieldItem<0, FIELD_INDEX>(ast, parsers...))
        {
            return ioState.Success();
        }
        CLEAR_NOTNULL(ast);
        return false;
    }
    
    template <typename AST_PTR = nullptr_t, size_t... FIELD_INDICES, typename... PARSERS>
    bool ParseSequenceFields(AST_PTR ast, std::index_sequence<FIELD_INDICES...> indices, PARSERS... parsers)
    {
        auto ioState(this->Save());
        if (ParseSequenceFieldsItem<0>(ast, indices, parsers...))
        {
            return ioState.Success();
        }
        CLEAR_NOTNULL(ast);
        return false;
    }

    template <typename ELEMS_PTR = nullptr_t, typename PARSER_HEAD, typename PARSER_OTHERS>
    bool ParseSequenceHeadThenOthers(ELEMS_PTR elems, PARSER_HEAD parserHead, PARSER_OTHERS parserOthers)
    {
        auto ioState(this->Save());
        size_t prevSize = GetSizeIfNotNull(elems);

        decltype(ElemTypeOrNull(elems)) elem = {};
        if (parserHead(PtrOrNull(elem)))
        {
            PushBackIfNotNull(elems, elem);

            if (parserOthers(elems))
            {
                return ioState.Success();
            }
        }

        ResizeIfNotNull(elems, prevSize);

        return false;
    }

    template <typename ELEMS_PTR, typename... PARSERS>
    bool ParseRepeatSequenceEx(ELEMS_PTR elems, size_t minCount, size_t maxCount, PARSERS... parsers)
    {
        size_t count = 0;
        size_t prevSize = GetSizeIfNotNull(elems);

        auto ioState(this->Save());

        for (; count < maxCount; ++count)
        {
            decltype(ElemTypeOrNull(elems)) elem = {};
            if (ParseSequence(PtrOrNull(elem), parsers...))
            {
                PushBackIfNotNull(elems, elem);
            }
            else
            {
                break;
            }
        }

        if (count < minCount)
        {
            ResizeIfNotNull(elems, prevSize);
            return false;
        }

        return ioState.Success();
    }
    
    template <typename ELEMS_PTR, typename... PARSERS>
    bool ParseRepeatSequence(ELEMS_PTR elems, PARSERS... parsers)
    {
        return ParseRepeatSequenceEx(elems, 0, SIZE_MAX, parsers...);
    }

    template <typename ELEMS_PTR, typename PARSER>
    bool ParseList(ELEMS_PTR list, PARSER parser)
    {
        // X-list    =   (X *("," X))
        return ParseSequenceHeadThenOthers(list, parser,
            THIS_PARSE_ARGS(ParseRepeat,
                THIS_PARSE_ARGS(ParseSequenceOneField<1>,
                    THIS_PARSE_ARGS(ParseCharExact, ',', true),
                    parser)));
    }

    // Core rules as defined in the Appendix B https://tools.ietf.org/html/rfc5234#appendix-B
    bool ParseVCHAR(nullptr_t = nullptr)
    {
        // VCHAR          =  %x21-7E
        return ParseChar([](auto ch) { return ch >= 0x10 && ch <= 0x7e; });
    }

    bool ParseWSP(nullptr_t = nullptr)
    {
        // WSP            =  SP / HTAB
        return ParseChar([](auto ch) { return ch == ' ' || ch == '\t'; });
    }

    bool ParseCRLF(nullptr_t = nullptr)
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
    using Base = ParserBase<INPUT, CHAR_TYPE>;
public:
    ParserRFC5322(INPUT input)
        : Base(input)
    {
    }

    // 3.2.2.  Folding White Space and Comments
    bool ParseFWS(nullptr_t = nullptr)
    {
        // FWS             =   ([*WSP CRLF] 1*WSP) /  obs-FWS
        do
        {
            if (!this->ParseRepeat(nullptr, THIS_PARSE(ParseWSP), 1))
            {
                return false;
            }
        }
        while (this->ParseCRLF());

        return true;
    }


    bool ParseCText(nullptr_t = nullptr)
    {
        // ctext           =   %d33-39 /          ; Printable US-ASCII
        //                     %d42-91 /          ;  characters not including
        //                     %d93-126 /         ;  "(", ")", or "\"
        //                     obs-ctext

        return this->ParseChar([&](auto ch)
        {
            if (ch >= 33 && ch <= 39)
                return true;
            if (ch >= 42 && ch <= 91)
                return true;
            if (ch >= 93 && ch <= 126)
                return true;
            return false;
        });
    }

    bool ParseQuotedPair(nullptr_t = nullptr)
    {
        // quoted-pair     = ("\" (VCHAR / WSP))
        if (this->input_.GetIf('\\'))
        {
            this->output_('\\', true);
            return (this->ParseVCHAR() || this->ParseWSP());
        }
        return false;
    }

    bool ParseCContent(nullptr_t = nullptr);

    bool ParseComment(nullptr_t = nullptr)
    {
        // comment         =   "(" *([FWS] ccontent) [FWS] ")"
        return this->ParseSequence(nullptr,
            THIS_PARSE_ARGS(ParseCharExact, '(', true),
            THIS_PARSE_ARGS(ParseRepeatSequence,
                THIS_PARSE_ARGS(ParseOptional, THIS_PARSE(ParseFWS)),
                THIS_PARSE(ParseCContent)),
            THIS_PARSE_ARGS(ParseOptional, THIS_PARSE(ParseFWS)),
            THIS_PARSE_ARGS(ParseCharExact, ')', true));
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
    bool ParseAText(nullptr_t = nullptr)
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
        return this->ParseChar([](auto ch)
        {
            if (ch >= 'a' && ch <= 'z')
                return true;
            if (ch >= 'A' && ch <= 'Z')
                return true;
            if (ch >= '0' && ch <= '9')
                return true;
            return std::string("!#$%&\'*+-/=?^_`{|}~").find(ch) != std::string::npos;
        });
    }
    
    template <typename PARSER>
    bool ParseTextBetweenComment(TextWithComm * result, PARSER parser)
    {
        // TextBetweenComment            =   [CFWS] parser [CFWS]
        auto ioState(this->Save());
        ParseCFWS(FIELD_NOTNULL(result, TextWithCommFields_CommentBefore));
        size_t c1 = this->output_.Pos();
        if (parser(nullptr))
        {
            size_t c2 = this->output_.Pos();
            ParseCFWS(FIELD_NOTNULL(result, TextWithCommFields_CommentAfter));

            if (result)
            {
                std::get<TextWithCommFields_Content>(*result) = std::make_pair(c1, c2);
            }

            return ioState.Success();
        }
        CLEAR_NOTNULL(result);
        return false;
    }

    bool ParseAtom(TextWithComm * atom = nullptr)
    {
        // atom            =   [CFWS] 1*atext [CFWS]
        return ParseTextBetweenComment(atom,
            THIS_PARSE_ARGS(ParseRepeat, THIS_PARSE(ParseAText), 1));
    }

    bool ParseDotAtomText(nullptr_t = nullptr)
    {
        // dot-atom-text   =   1*atext *("." 1*atext)
        return this->ParseSequence(nullptr,
            THIS_PARSE_ARGS(ParseRepeat, THIS_PARSE(ParseAText), 1),
            THIS_PARSE_ARGS(ParseRepeatSequence,
                THIS_PARSE_ARGS(ParseCharExact, '.', false),
                THIS_PARSE_ARGS(ParseRepeat, THIS_PARSE(ParseAText), 1)));
    }

    bool ParseDotAtom(TextWithComm * dotAtom = nullptr)
    {
        // dot-atom        =   [CFWS] dot-atom-text [CFWS]
        return ParseTextBetweenComment(dotAtom, THIS_PARSE(ParseDotAtomText));
    }

    bool ParseSpecials(nullptr_t = nullptr)
    {
        // specials        =   "(" / ")" /        ; Special characters that do
        //                     "<" / ">" /        ;  not appear in atext
        //                     "[" / "]" /
        //                     ":" / ";" /
        //                     "@" / "\" /
        //                     "," / "." /
        //                     DQUOTE
        return this->ParseChar([](auto ch) { return std::string("()<>[]:;@\\,/\"").find(ch) != std::string::npos; });
    }

    bool ParseQText(nullptr_t = nullptr)
    {
       // qtext           =   %d33 /             ; Printable US-ASCII
       //                     %d35-91 /          ;  characters not including
       //                     %d93-126 /         ;  "\" or the quote character
       //                     obs-qtext
        return this->ParseChar([](auto ch) { return ch == 33 || (ch >= 35 && ch <= 91) || (ch >= 93 && ch <= 126); });
    }

    bool ParseQContent(nullptr_t = nullptr)
    {
        // qcontent        =   qtext / quoted-pair
        return ParseQText() || ParseQuotedPair();
    }

    bool ParseOptFWS(nullptr_t = nullptr)
    {
        // [FWS]
        return this->ParseOptional(nullptr, THIS_PARSE(ParseFWS));
    }

    bool ParseSeqOptFWSQContent(nullptr_t = nullptr)
    {
        // *([FWS] qcontent)
        return this->ParseRepeatSequence(nullptr, THIS_PARSE(ParseOptFWS), THIS_PARSE(ParseQContent));
    }

    bool ParseQuotedString(TextWithComm * quotedString)
    {
        // quoted-string   =   [CFWS]
        //                     DQUOTE *([FWS] qcontent) [FWS] DQUOTE
        //                     [CFWS]
        return this->ParseTextBetweenComment(quotedString,
            THIS_PARSE_ARGS(ParseSequence,
                THIS_PARSE_ARGS(ParseCharExact, '\"', true),
                THIS_PARSE(ParseSeqOptFWSQContent),
                THIS_PARSE(ParseOptFWS),
                THIS_PARSE_ARGS(ParseCharExact, '\"', true)));
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
        return this->ParseRepeat(phrase, THIS_PARSE(ParseWord), 1);
    }

    // 3.4.1.  Addr-Spec Specification

    bool ParseLocalPart(TextWithComm * localPart)
    {
        // local-part      =   dot-atom / quoted-string / obs-local-part
        return ParseDotAtom(localPart) || ParseQuotedString(localPart);
    }

    bool ParseDText(nullptr_t)
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

    bool ParseSeqOptFWSDText(nullptr_t)
    {
        // *([FWS] dtext)
        return this->ParseRepeatSequence(nullptr,
            THIS_PARSE(ParseOptFWS),
            THIS_PARSE(ParseDText));
    }

    bool ParseDomainLiteral(TextWithComm * domainLiteral)
    {
        // domain-literal  =   [CFWS] "[" *([FWS] dtext) [FWS] "]" [CFWS]

        return this->ParseTextBetweenComment(domainLiteral,
            THIS_PARSE_ARGS(ParseSequence,
                THIS_PARSE_ARGS(ParseCharExact, '[', true),
                THIS_PARSE(ParseSeqOptFWSDText),
                THIS_PARSE(ParseOptFWS),
                THIS_PARSE_ARGS(ParseCharExact, ']', true)));
    }

    bool ParseDomain(TextWithComm * domain)
    {
        // domain          =   dot-atom / domain-literal / obs-domain
        return ParseDotAtom(domain) || ParseDomainLiteral(domain);
    }

    bool ParseAddrSpec(AddrSpec * addrSpec)
    {
        // addr-spec       =   local-part "@" domain
        return this->ParseSequenceFields(addrSpec,
            std::index_sequence<AddrSpecFields_LocalPart, SIZE_MAX, AddrSpecFields_DomainPart>(),
            THIS_PARSE(ParseLocalPart),
            THIS_PARSE_ARGS(ParseCharExact, '@', true),
            THIS_PARSE(ParseDomain));
    }

    // 3.4.  Address Specification
    bool ParseAngleAddr(AngleAddr * angleAddr)
    {
        // angle-addr      =   [CFWS] "<" addr-spec ">" [CFWS] /
        //                 obs-angle-addr
        return ParseSequenceFields(angleAddr,
            std::index_sequence<AngleAddrFields_CommentBefore, SIZE_MAX, AngleAddrFields_Content, SIZE_MAX, AngleAddrFields_CommentAfter>(),
            THIS_PARSE_ARGS(ParseOptional, THIS_PARSE(ParseCFWS)),
            THIS_PARSE_ARGS(ParseCharExact, '<', true),
            THIS_PARSE(ParseAddrSpec),
            THIS_PARSE_ARGS(ParseCharExact, '>', true),
            THIS_PARSE_ARGS(ParseOptional, THIS_PARSE(ParseCFWS)));
    }

    bool ParseDisplayName(MultiTextWithComm * displayName)
    {
        // display-name    =   phrase
        return ParsePhrase(displayName);
    }

    bool ParseNameAddr(Mailbox * nameAddr)
    {
        // name-addr       =   [display-name] angle-addr
        return this->ParseSequence(nameAddr,
            THIS_PARSE_ARGS(ParseOptional, THIS_PARSE(ParseDisplayName)),
            THIS_PARSE(ParseAngleAddr));
    }

    bool ParseMailbox(Mailbox * mailbox)
    {
        // mailbox         =   name-addr / addr-spec
        return ParseNameAddr(mailbox) || ParseAddrSpec(FIELD_NOTNULL(FIELD_NOTNULL(mailbox, MailboxFields_Address), AngleAddrFields_Content));
    }

    bool ParseMailboxList(MailboxList * mailboxList)
    {
        // mailbox-list    =   (mailbox *("," mailbox)) / obs-mbox-list
        return this->ParseList(mailboxList, THIS_PARSE(ParseMailbox));
    }

    bool ParseGroupList(GroupList * group)
    {
        // group-list      =   mailbox-list / CFWS / obs-group-list
        return ParseMailboxList(FIELD_NOTNULL(group, GroupListFields_Mailboxes)) || ParseCFWS(FIELD_NOTNULL(group, GroupListFields_Comment));
    }

    bool ParseGroup(Group * group)
    {
        // group           =   display-name ":" [group-list] ";" [CFWS]
        return this->ParseSequenceFields(group,
            std::index_sequence<GroupFields_DisplayName, SIZE_MAX, GroupFields_GroupList, SIZE_MAX, GroupFields_Comment>(),
            THIS_PARSE(ParseDisplayName),
            THIS_PARSE_ARGS(ParseCharExact, ':', true),
            THIS_PARSE(ParseGroupList),
            THIS_PARSE_ARGS(ParseCharExact, ';', true),
            THIS_PARSE_ARGS(ParseOptional, THIS_PARSE(ParseCFWS)));
    }

    bool ParseAddress(Address * address)
    {
        // address         =   mailbox / group
        return ParseMailbox(FIELD_NOTNULL(address, AddressFields_Mailbox)) || ParseGroup(FIELD_NOTNULL(address, AddressFields_Group));
    }

    bool ParseAddressList(AddressList * addresses)
    {
        // address-list    =   (address *("," address)) / obs-addr-list
        return this->ParseList(addresses, THIS_PARSE(ParseAddress));
    }
};

template <typename INPUT, typename CHAR_TYPE>
bool ParserRFC5322<INPUT, CHAR_TYPE>::ParseCContent(nullptr_t)
{
    // ccontent        =   ctext / quoted-pair / comment
    return ParseCText() || ParseQuotedPair() || ParseComment();
}

template <typename INPUT, typename CHAR_TYPE>
ParserRFC5322<INPUT, CHAR_TYPE> Make_ParserRFC5322(INPUT && input, CHAR_TYPE charType)
{
    return ParserRFC5322<INPUT, CHAR_TYPE>(input);
}

#include <iostream>

void test_address(std::string const & addr)
{
    std::cout << addr;

    auto parser(Make_ParserRFC5322([&, pos = (size_t)0] () mutable
    {
        if (pos < addr.size())
            return (int)addr[pos++];
        return EOF;
    }, (char)0));

    AddressList addresses;
    if (parser.ParseAddressList(&addresses) && parser.Ended())
    {
        auto const & outBuffer = parser.OutputBuffer();
        std::cout << " is OK\n";
        std::cout << addresses.size() << " address" << (addresses.size() > 1 ? "es" : "") << " parsed." << std::endl;
        for (auto const & address : addresses)
        {
            auto displayMailBox = [&](auto const & mailbox, auto indent)
            {
                std::cout << indent << "   Mailbox:" << std::endl;
                std::cout << indent << "     Display Name: '" << ToString(outBuffer, true, std::get<MailboxFields_DisplayName>(mailbox)) << "'" << std::endl;
                std::cout << indent << "     Adress: " << std::endl;
                auto const & addrSpec = std::get<AngleAddrFields_Content>(std::get<MailboxFields_Address>(mailbox));
                std::cout << indent << "       Local-Part: '" << ToString(outBuffer, false, std::get<AddrSpecFields_LocalPart>(addrSpec)) << "'" << std::endl;
                std::cout << indent << "       Domain-Part: '" << ToString(outBuffer, false, std::get<AddrSpecFields_DomainPart>(addrSpec)) << "'" << std::endl;
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
    test_address("arobar     d <sigma@addr.net>");
    test_address("troll@bitch.com");
    test_address("troll@bitch.com, arobar     d <sigma@addr.net>, sir john snow <user.name+tag+sorting@example.com(comment)>");
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
