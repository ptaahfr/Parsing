#pragma once

#include "ParserCore.hpp"
#include "ParserRFC5322Data.hpp"

// Rules defined in https://tools.ietf.org/html/rfc5322

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
static auto const AText = CharPred([](auto ch)
{
    if (ch >= 'a' && ch <= 'z')
        return true;
    if (ch >= 'A' && ch <= 'Z')
        return true;
    if (ch >= '0' && ch <= '9')
        return true;
    return std::string("!#$%&\'*+-/=?^_`{|}~").find(ch) != std::string::npos;
});

// ctext           =   %d33-39 /          ; Printable US-ASCII
//                     %d42-91 /          ;  characters not including
//                     %d93-126 /         ;  "(", ")", or "\"
//                     obs-ctext
static auto const CText = CharPred([](auto ch)
{
    if (ch >= 33 && ch <= 39)
        return true;
    if (ch >= 42 && ch <= 91)
        return true;
    if (ch >= 93 && ch <= 126)
        return true;
    return false;
});

// dtext           =  %d33-90 /          ; Printable US-ASCII
//                    %d94-126 /         ;  characters not including
//                    obs-dtext          ;  "[", "]", or "\"
static auto const DText = CharPred([](auto ch)
{
    return (ch >= 33 && ch <= 90) || (ch >= 94 && ch <= 126);
});

// specials        =   "(" / ")" /        ; Special characters that do
//                     "<" / ">" /        ;  not appear in atext
//                     "[" / "]" /
//                     ":" / ";" /
//                     "@" / "\" /
//                     "," / "." /
//                     DQUOTE
static auto const Specials = CharPred([](auto ch)
{
    return std::string("()<>[]:;@\\,/\"").find(ch) != std::string::npos;
});

// qtext           =   %d33 /             ; Printable US-ASCII
//                     %d35-91 /          ;  characters not including
//                     %d93-126 /         ;  "\" or the quote character
//                     obs-qtext
static auto const QText = CharPred([](auto ch) { return ch == 33 || (ch >= 35 && ch <= 91) || (ch >= 93 && ch <= 126); });

DECLARE_PARSER_PRIMITIVE(FWS)
DECLARE_PARSER_PRIMITIVE(QuotedPair)
DECLARE_PARSER_PRIMITIVE(CContent)
DECLARE_PARSER_PRIMITIVE(Comment)
DECLARE_PARSER_PRIMITIVE(CFWS)
DECLARE_PARSER_PRIMITIVE(Atom)
DECLARE_PARSER_PRIMITIVE(DotAtomText)
DECLARE_PARSER_PRIMITIVE(DotAtom)
DECLARE_PARSER_PRIMITIVE(QContent)
DECLARE_PARSER_PRIMITIVE(QuotedString)
DECLARE_PARSER_PRIMITIVE(Word)
DECLARE_PARSER_PRIMITIVE(Phrase)
DECLARE_PARSER_PRIMITIVE(LocalPart)
DECLARE_PARSER_PRIMITIVE(DomainLiteral)
DECLARE_PARSER_PRIMITIVE(Domain)
DECLARE_PARSER_PRIMITIVE(AddrSpec)
DECLARE_PARSER_PRIMITIVE(AngleAddr)
DECLARE_PARSER_PRIMITIVE(NameAddr)
DECLARE_PARSER_PRIMITIVE(DomainAddr)
DECLARE_PARSER_PRIMITIVE(Mailbox)
DECLARE_PARSER_PRIMITIVE(MailboxList)
DECLARE_PARSER_PRIMITIVE(Group)
DECLARE_PARSER_PRIMITIVE(GroupList)
DECLARE_PARSER_PRIMITIVE(Address)
DECLARE_PARSER_PRIMITIVE(AddressList)

// display-name    =   phrase
static auto const DisplayName = Phrase;

// Temporary primitives
DECLARE_PARSER_PRIMITIVE(SeqOptFWSDText)
DECLARE_PARSER_PRIMITIVE(SeqOptFWSQContent)
DECLARE_PARSER_PRIMITIVE(OptFWS)

template <typename INPUT, typename CHAR_TYPE = char>
class ParserRFC5322 : public ParserCore<INPUT, CHAR_TYPE>
{
    using Base = ParserCore<INPUT, CHAR_TYPE>;
public:
    ParserRFC5322(INPUT input)
        : Base(input)
    {
    }

    template <typename... ARGS>
    auto Parse(ARGS &&... args) -> decltype(Base::Parse(args...))
    {
        return Base::Parse(args...);
    }
    
    template <typename... ARGS>
    auto Parse(ARGS &&... args) -> decltype(Base::Parse(*this, args...))
    {
        return Base::Parse(*this, args...);
    }

    // 3.2.2.  Folding White Space and Comments
    bool Parse(nullptr_t, FWSType)
    {
        // FWS             =   ([*WSP CRLF] 1*WSP) /  obs-FWS
        do
        {
            if (!this->Parse(nullptr, Repeat<1>(WSP)))
            {
                return false;
            }
        }
        while (this->Parse(nullptr, CRLF));

        return true;
    }

    bool Parse(nullptr_t, QuotedPairType)
    {
        // quoted-pair     = ("\" (VCHAR / WSP))
        if (this->input_.GetIf('\\'))
        {
            this->output_('\\', true);
            return (this->Parse(nullptr, VCHAR) || this->Parse(nullptr, WSP));
        }
        return false;
    }

    bool Parse(nullptr_t, CContentType);

    bool Parse(nullptr_t, CommentType)
    {
        // comment         =   "(" *([FWS] ccontent) [FWS] ")"
        return this->Parse(nullptr, Sequence(CharExact('('), Repeat(Sequence(Optional(FWS), CContent)), Optional(FWS), CharExact(')')));
    }

    bool Parse(SubstringPos * comment, CFWSType)
    {
        // CFWS            =   (1*([FWS] comment) [FWS]) / FWS
        bool valid = false;

        if (comment)
            comment->first = this->output_.Pos();
        for (;;)
        {
            if (Parse(nullptr, FWS))
            {
                valid = true;
            }
            if (Parse(nullptr, Comment))
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
    
    template <typename PARSER>
    bool ParseTextBetweenComment(TextWithCommData * result, PARSER parser)
    {
        // TextBetweenComment            =   [CFWS] parser [CFWS]
        auto ioState(this->Save());
        Parse(FIELD_NOTNULL(result, TextWithCommFields_CommentBefore), CFWS);
        size_t c1 = this->output_.Pos();
        if (parser(nullptr))
        {
            size_t c2 = this->output_.Pos();
            Parse(FIELD_NOTNULL(result, TextWithCommFields_CommentAfter), CFWS);

            if (result)
            {
                std::get<TextWithCommFields_Content>(*result) = std::make_pair(c1, c2);
            }

            return ioState.Success();
        }
        CLEAR_NOTNULL(result);
        return false;
    }

    bool Parse(TextWithCommData * atom, AtomType)
    {
        // atom            =   [CFWS] 1*atext [CFWS]
        return ParseTextBetweenComment(atom, PARSE(Repeat<1>(AText)));
    }

    bool Parse(nullptr_t, DotAtomTextType)
    {
        // dot-atom-text   =   1*atext *("." 1*atext)
        return this->Parse(nullptr, Sequence(Repeat<1>(AText), Repeat(Sequence(CharExact('.'), Repeat<1>(AText)))));
    }

    bool Parse(TextWithCommData * dotAtom, DotAtomType)
    {
        // dot-atom        =   [CFWS] dot-atom-text [CFWS]
        return ParseTextBetweenComment(dotAtom, PARSE(DotAtomText));
    }

    bool Parse(nullptr_t, QContentType)
    {
        // qcontent        =   qtext / quoted-pair
        return Parse(nullptr, QText) || Parse(nullptr, QuotedPair);
    }

    //bool Parse(nullptr_t, OptFWSType)
    //{
    //    // [FWS]
    //    return this->ParseOptional(nullptr, PARSE(FWS));
    //}

    //bool Parse(nullptr_t, SeqOptFWSQContentType)
    //{
    //    // *([FWS] qcontent)
    //    return this->Parse(nullptr, Repeat(Sequence(OptFWS, QContent)));
    //}

    bool Parse(TextWithCommData * quotedString, QuotedStringType)
    {
        // quoted-string   =   [CFWS]
        //                     DQUOTE *([FWS] qcontent) [FWS] DQUOTE
        //                     [CFWS]
        return this->ParseTextBetweenComment(quotedString,
            PARSE(Sequence(CharExact('\"'), Repeat(Sequence(Optional(FWS), QContent)), Optional(FWS), CharExact('\"'))));
    }

    // 3.2.5.  Miscellaneous Tokens
    bool Parse(TextWithCommData * word, WordType)
    {
        // word            =   atom / quoted-string
        return Parse(word, Atom) || Parse(word, QuotedString);
    }

    bool Parse(MultiTextWithComm * phrase, PhraseType)
    {
        // phrase          =   1*word / obs-phrase
        return this->ParseRepeat(phrase, PARSE(Word), 1);
    }

    // 3.4.1.  Addr-Spec Specification
    bool Parse(TextWithCommData * localPart, LocalPartType)
    {
        // local-part      =   dot-atom / quoted-string / obs-local-part
        return Parse(localPart, DotAtom) || Parse(localPart, QuotedString);
    }

    bool Parse(nullptr_t, SeqOptFWSDTextType)
    {
        // *([FWS] dtext)
        return this->Parse(nullptr, Repeat(Sequence(Optional(FWS), DText)));
    }

    bool Parse(TextWithCommData * domainLiteral, DomainLiteralType)
    {
        // domain-literal  =   [CFWS] "[" *([FWS] dtext) [FWS] "]" [CFWS]

        return this->ParseTextBetweenComment(domainLiteral,
            PARSE(Sequence(
                CharExact('['),
                Repeat(Sequence(
                    Optional(FWS), DText)),
                Optional(FWS),
                CharExact(']')))
            );
    }

    bool Parse(TextWithCommData * domain, DomainType)
    {
        // domain          =   dot-atom / domain-literal / obs-domain
        return Parse(domain, DotAtom) || Parse(domain, DomainLiteral);
    }

    bool Parse(AddrSpecData * addrSpec, AddrSpecType)
    {
        // addr-spec       =   local-part "@" domain
        return this->Parse(addrSpec, IndexedSequence(
            std::index_sequence<AddrSpecFields_LocalPart<AddrSpecFields_LocalPart, INDEX_NONE, AddrSpecFields_DomainPart>(),
            LocalPart, CharExact('@'), Domain));
    }

    // 3.4.  Address Specification
    bool Parse(AngleAddrData * angleAddr, AngleAddrType)
    {
        // angle-addr      =   [CFWS] "<" addr-spec ">" [CFWS] /
        //                 obs-angle-addr
        return this->Parse(angleAddr, IndexedSequence(
            std::index_sequence<AngleAddrFields_CommentBefore, INDEX_NONE, AngleAddrFields_Content, INDEX_NONE, AngleAddrFields_CommentAfter>(),
            Optional(CFWS), CharExact('<'), AddrSpec, CharExact('>'), Optional(CFWS)));
    }

    bool Parse(NameAddrData * nameAddr, NameAddrType)
    {
        // name-addr       =   [display-name] angle-addr
        return this->Parse(nameAddr, Sequence(Optional(DisplayName), AngleAddr));
    }

    bool Parse(MailboxData * mailbox, MailboxType)
    {
        // mailbox         =   name-addr / addr-spec
        return Parse(mailbox, Alternatives(NameAddr, AddrSpec));
    }

    template <typename ELEMS_PTR, typename PRIMITIVE>
    bool ParseList(ELEMS_PTR list, PRIMITIVE primitive)
    {
        // X-list    =   (X *("," X))
        return Parse(list,
            IndexedSequence(std::index_sequence<INDEX_THIS, INDEX_THIS>(),
                Head(primitive),
                Repeat(IndexedSequence(std::index_sequence<INDEX_NONE, INDEX_THIS>(), CharExact(','), primitive))));
    }

    bool Parse(MailboxListData * mailboxList, MailboxListType)
    {
        // mailbox-list    =   (mailbox *("," mailbox)) / obs-mbox-list
        return this->ParseList(mailboxList, Mailbox);
    }

    bool Parse(GroupListData * group, GroupListType)
    {
        // group-list      =   mailbox-list / CFWS / obs-group-list
        return Parse(group, IndexedAlternatives(
            std::index_sequence<GroupListFields_Mailboxes, GroupListFields_Comment>(),
            MailboxList, CFWS));
    }

    bool Parse(GroupData * group, GroupType)
    {
        // group           =   display-name ":" [group-list] ";" [CFWS]
        return this->Parse(group,
            IndexedSequence(
                std::index_sequence<GroupFields_DisplayName, SIZE_MAX, GroupFields_GroupList, SIZE_MAX, GroupFields_Comment>(),
                DisplayName, CharExact(':'), GroupList, CharExact(';'), Optional(CFWS)));
    }

    bool Parse(AddressData * address, AddressType)
    {
        // address         =   mailbox / group
        return Parse(address, Alternatives(Mailbox, Group));
    }

    bool Parse(AddressListData * addresses, AddressListType)
    {
        // address-list    =   (address *("," address)) / obs-addr-list
        return this->ParseList(addresses, Address);
    }
};

template <typename INPUT, typename CHAR_TYPE>
bool ParserRFC5322<INPUT, CHAR_TYPE>::Parse(nullptr_t, CContentType)
{
    // ccontent        =   ctext / quoted-pair / comment
    return Parse(nullptr, Alternatives(CText, QuotedPair, Comment));
}

template <typename INPUT, typename CHAR_TYPE>
ParserRFC5322<INPUT, CHAR_TYPE> Make_ParserRFC5322(INPUT && input, CHAR_TYPE charType)
{
    return ParserRFC5322<INPUT, CHAR_TYPE>(input);
}
