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

// 3.2.2.  Folding White Space and Comments

// FWS             =   ([*WSP CRLF] 1*WSP) /  obs-FWS
static auto const FWS = Sequence(Optional(Repeat(WSP), CRLF), Repeat<1>(WSP));

// quoted-pair     = ("\" (VCHAR / WSP))
static auto const QuotedPair = Sequence(CharExact('\\'), Alternatives(VCHAR, WSP));

// We are forced to declare a custom primitive for CContent because of circular reference
PARSER_CUSTOM_PRIMITIVE(CContent)

// comment         =   "(" *([FWS] ccontent) [FWS] ")"
static auto const Comment = Sequence(CharExact('('), Repeat(Optional(FWS), CContent), Optional(FWS), CharExact(')'));

// CFWS            =   (1*([FWS] comment) [FWS]) / FWS
static auto const CFWS = Alternatives(Repeat<1>(Optional(FWS), Comment), Optional(FWS), FWS);

// atom            =   [CFWS] 1*atext [CFWS]
static auto const Atom = Sequence(Optional(CFWS), Repeat<1>(AText), Optional(CFWS));

// dot-atom-text   =   1*atext *("." 1*atext)
static auto const DotAtomText = Sequence(Repeat<1>(AText), Repeat(CharExact('.'), Repeat<1>(AText)));

// dot-atom        =   [CFWS] dot-atom-text [CFWS]
static auto const DotAtom = Sequence(Optional(CFWS), DotAtomText, Optional(CFWS));

// qcontent        =   qtext / quoted-pair
static auto const QContent = Alternatives(QText, QuotedPair);

// quoted-string   =   [CFWS]
//                     DQUOTE *([FWS] qcontent) [FWS] DQUOTE
//                     [CFWS]
static auto const QuotedString = IndexedSequence(
    std::index_sequence<TextWithCommFields_CommentBefore, INDEX_NONE, TextWithCommFields_CommentBefore, INDEX_NONE, TextWithCommFields_CommentAfter>(),
    Optional(CFWS), CharExact('\"'), Sequence(Repeat(Optional(FWS), QContent), Optional(FWS)), CharExact('\"'), Optional(CFWS));

// 3.2.5.  Miscellaneous Tokens

// word            =   atom / quoted-string
static auto const Word = Alternatives(Atom, QuotedString);

// phrase          =   1*word / obs-phrase
static auto const Phrase = Repeat<1>(Word);

// display-name    =   phrase
static auto const DisplayName = Phrase;

// 3.4.1.  Addr-Spec Specification

// local-part      =   dot-atom / quoted-string / obs-local-part
static auto const LocalPart = Alternatives(DotAtom, QuotedString);

// domain-literal  =   [CFWS] "[" *([FWS] dtext) [FWS] "]" [CFWS]
static auto const DomainLiteral = IndexedSequence(
    std::index_sequence<TextWithCommFields_CommentBefore, INDEX_NONE, TextWithCommFields_CommentBefore, INDEX_NONE, TextWithCommFields_CommentAfter>(),
    Optional(CFWS), CharExact('['), Sequence(Repeat(Optional(FWS), DText), Optional(FWS)), CharExact(']'), Optional(CFWS));

// domain          =   dot-atom / domain-literal / obs-domain
static auto const Domain = Alternatives(DotAtom, DomainLiteral);

// addr-spec       =   local-part "@" domain
static auto const AddrSpec = IndexedSequence(
            std::index_sequence<AddrSpecFields_LocalPart, INDEX_NONE, AddrSpecFields_DomainPart>(),
            LocalPart, CharExact('@'), Domain);

// 3.4.  Address Specification

// angle-addr      =   [CFWS] "<" addr-spec ">" [CFWS] /
//                 obs-angle-addr
static auto const AngleAddr = IndexedSequence(
            std::index_sequence<AngleAddrFields_CommentBefore, INDEX_NONE, AngleAddrFields_Content, INDEX_NONE, AngleAddrFields_CommentAfter>(),
            Optional(CFWS), CharExact('<'), AddrSpec, CharExact('>'), Optional(CFWS));

// name-addr       =   [display-name] angle-addr
static auto const NameAddr = Sequence(Optional(DisplayName), AngleAddr);

// mailbox         =   name-addr / addr-spec
static auto const Mailbox = IndexedAlternatives(std::index_sequence<MailboxFields_NameAddr, MailboxFields_AddrSpec>(), NameAddr, AddrSpec);

// mailbox-list    =   (mailbox *("," mailbox)) / obs-mbox-list
static auto const MailboxList = IndexedSequence(std::index_sequence<INDEX_THIS, INDEX_THIS>(),
                Head(Mailbox),
                Repeat(IndexedSequence(std::index_sequence<INDEX_NONE, INDEX_THIS>(), CharExact(','), Mailbox)));

// group-list      =   mailbox-list / CFWS / obs-group-list
static auto const GroupList = IndexedAlternatives(std::index_sequence<GroupListFields_Mailboxes, GroupListFields_Comment>(), MailboxList, CFWS);

// group           =   display-name ":" [group-list] ";" [CFWS]
static auto const Group = IndexedSequence(std::index_sequence<GroupFields_DisplayName, INDEX_NONE, GroupFields_GroupList, INDEX_NONE, GroupFields_Comment>(),
                DisplayName, CharExact(':'), GroupList, CharExact(';'), Optional(CFWS));

// address         =   mailbox / group
static auto const Address = IndexedAlternatives(
            std::index_sequence<AddressFields_Mailbox, AddressFields_Group>(), Mailbox, Group);

// address-list    =   (address *("," address)) / obs-addr-list
static auto const AddressList = IndexedSequence(std::index_sequence<INDEX_THIS, INDEX_THIS>(),
                Head(Address),
                Repeat(IndexedSequence(std::index_sequence<INDEX_NONE, INDEX_THIS>(), CharExact(','), Address)));

// Currently need to have the entry point as custom overload
//DECLARE_PARSER_PRIMITIVE(AddressList)


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

    bool Parse(nullptr_t, CContentType)
    {
        // ccontent        =   ctext / quoted-pair / comment
        return Parse(nullptr, Alternatives(CText, QuotedPair, Comment));
    }

    //bool Parse(AddressListData * addresses, AddressListType)
    //{
    //    // address-list    =   (address *("," address)) / obs-addr-list
    //    return Parse(addresses,
    //        IndexedSequence(std::index_sequence<INDEX_THIS, INDEX_THIS>(),
    //            Head(Address),
    //            Repeat(IndexedSequence(std::index_sequence<INDEX_NONE, INDEX_THIS>(), CharExact(','), Address))));
    //}
};

template <typename INPUT, typename CHAR_TYPE>
ParserRFC5322<INPUT, CHAR_TYPE> Make_ParserRFC5322(INPUT && input, CHAR_TYPE charType)
{
    return ParserRFC5322<INPUT, CHAR_TYPE>(input);
}
