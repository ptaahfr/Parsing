// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234
//
// rules definitions for the email address parsing
#pragma once

#include "ParserCore.hpp"
#include "ParserRFC5322Data.hpp"

namespace RFC5322
{
using namespace RFC5234Core;

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

PARSER_RULE(AText, CharPred([](auto ch)
{
    if (ch >= 'a' && ch <= 'z')
        return true;
    if (ch >= 'A' && ch <= 'Z')
        return true;
    if (ch >= '0' && ch <= '9')
        return true;
    return std::string("!#$%&\'*+-/=?^_`{|}~").find(ch) != std::string::npos;
}))

// ctext           =   %d33-39 /          ; Printable US-ASCII
//                     %d42-91 /          ;  characters not including
//                     %d93-126 /         ;  "(", ")", or "\"
//                     obs-ctext
PARSER_RULE(CText, CharPred([](auto ch)
{
    if (ch >= 33 && ch <= 39)
        return true;
    if (ch >= 42 && ch <= 91)
        return true;
    if (ch >= 93 && ch <= 126)
        return true;
    return false;
}))

// dtext           =  %d33-90 /          ; Printable US-ASCII
//                    %d94-126 /         ;  characters not including
//                    obs-dtext          ;  "[", "]", or "\"
PARSER_RULE(DText, CharPred([](auto ch)
{
    return (ch >= 33 && ch <= 90) || (ch >= 94 && ch <= 126);
}))

// specials        =   "(" / ")" /        ; Special characters that do
//                     "<" / ">" /        ;  not appear in atext
//                     "[" / "]" /
//                     ":" / ";" /
//                     "@" / "\" /
//                     "," / "." /
//                     DQUOTE
PARSER_RULE(Specials, CharPred([](auto ch)
{
    return std::string("()<>[]:;@\\,/\"").find(ch) != std::string::npos;
}))

// qtext           =   %d33 /             ; Printable US-ASCII
//                     %d35-91 /          ;  characters not including
//                     %d93-126 /         ;  "\" or the quote character
//                     obs-qtext
PARSER_RULE(QText, CharPred([](auto ch) { return ch == 33 || (ch >= 35 && ch <= 91) || (ch >= 93 && ch <= 126); }))

// 3.2.2.  Folding White Space and Comments

// FWS             =   ([*WSP CRLF] 1*WSP) /  obs-FWS
PARSER_RULE(FWS, Sequence(Optional(Repeat(WSP()), CRLF()), Repeat<1>(WSP())))

// quoted-pair     = ("\" (VCHAR / WSP))
PARSER_RULE(QuotedPair, Sequence(CharExact('\\'), Alternatives(VCHAR(), WSP())))

PARSER_RULE_FORWARD(CContent)

// comment         =   "(" *([FWS] ccontent) [FWS] ")"
PARSER_RULE(Comment, Sequence(CharExact('('), Repeat(Optional(FWS()), CContent()), Optional(FWS()), CharExact(')')))

// ccontent        =   ctext / quoted-pair / comment
PARSER_RULE_PARTIAL(CContent, Alternatives(CText(), QuotedPair(), Comment()))

// CFWS            =   (1*([FWS] comment) [FWS]) / FWS
PARSER_RULE(CFWS, Alternatives(Repeat<1>(Optional(FWS()), Comment()), Optional(FWS()), FWS()))

// atom            =   [CFWS] 1*atext [CFWS]
PARSER_RULE(Atom, Sequence(Optional(CFWS()), Repeat<1>(AText()), Optional(CFWS())))

// dot-atom-text   =   1*atext *("." 1*atext)
PARSER_RULE(DotAtomText, Sequence(Repeat<1>(AText()), Repeat(CharExact('.'), Repeat<1>(AText()))))

// dot-atom        =   [CFWS] dot-atom-text [CFWS]
PARSER_RULE(DotAtom, Sequence(Optional(CFWS()), DotAtomText(), Optional(CFWS())))

// qcontent        =   qtext / quoted-pair
PARSER_RULE(QContent, Alternatives(QText(), QuotedPair()))

// quoted-string   =   [CFWS]
//                     DQUOTE *([FWS] qcontent) [FWS] DQUOTE
//                     [CFWS]
PARSER_RULE(QuotedString, Sequence(
    Idx<TextWithCommFields_CommentBefore>(), Optional(CFWS()),
    Idx<INDEX_NONE>(), CharExact('\"'), Idx<TextWithCommFields_Content>(), Sequence(Repeat(Optional(FWS()), QContent()), Optional(FWS())), Idx<INDEX_NONE>(), CharExact('\"'),
    Idx<TextWithCommFields_CommentAfter>(), Optional(CFWS())))

// 3.2.5.  Miscellaneous Tokens

// word            =   atom / quoted-string
PARSER_RULE(Word, Alternatives(Atom(), QuotedString()));

// phrase          =   1*word / obs-phrase
PARSER_RULE(Phrase, Repeat<1>(Word()))

// display-name    =   phrase
PARSER_RULE(DisplayName, Phrase())

// 3.4.1.  Addr-Spec Specification

// local-part      =   dot-atom / quoted-string / obs-local-part
PARSER_RULE(LocalPart, Alternatives(DotAtom(), QuotedString()))

// domain-literal  =   [CFWS] "[" *([FWS] dtext) [FWS] "]" [CFWS]
PARSER_RULE(DomainLiteral, Sequence(
    Idx<TextWithCommFields_CommentBefore>(), Optional(CFWS()),
    Idx<INDEX_NONE>(), CharExact('['),
    Idx<TextWithCommFields_Content>(), Sequence(Repeat(Optional(FWS()), DText()), Optional(FWS())),
    Idx<INDEX_NONE>(), CharExact(']'),
    Idx<TextWithCommFields_CommentAfter>(), Optional(CFWS())))

// domain          =   dot-atom / domain-literal / obs-domain
PARSER_RULE(Domain, Alternatives(DotAtom(), DomainLiteral()))

// addr-spec       =   local-part "@" domain
PARSER_RULE_DATA(AddrSpec, Sequence(
    Idx<AddrSpecFields_LocalPart>(), LocalPart(), Idx<INDEX_NONE>(), CharExact('@'), Idx<AddrSpecFields_DomainPart>(), Domain()))

// 3.4.  Address Specification

// angle-addr      =   [CFWS] "<" addr-spec ">" [CFWS] /
//                 obs-angle-addr
PARSER_RULE_DATA(AngleAddr, Sequence(
    Idx<AngleAddrFields_CommentBefore>(), Optional(CFWS()), Idx<INDEX_NONE>(), CharExact('<'), Idx<AngleAddrFields_Content>(), AddrSpec(), Idx<INDEX_NONE>(), CharExact('>'), Idx<AngleAddrFields_CommentAfter>(), Optional(CFWS())))

// name-addr       =   [display-name] angle-addr
PARSER_RULE_DATA(NameAddr, Sequence(Optional(DisplayName()), AngleAddr()))

// mailbox         =   name-addr / addr-spec
PARSER_RULE_DATA(Mailbox, Alternatives(Idx<MailboxFields_NameAddr>(), NameAddr(), Idx<MailboxFields_AddrSpec>(), AddrSpec()))

// mailbox-list    =   (mailbox *("," mailbox)) / obs-mbox-list
PARSER_RULE_DATA(MailboxList, Sequence(
    Idx<INDEX_THIS>(), Head(Mailbox()), Idx<INDEX_THIS>(), Repeat(Sequence(Idx<INDEX_NONE>(), CharExact(','), Idx<INDEX_THIS>(), Mailbox()))))

// group-list      =   mailbox-list / CFWS / obs-group-list
PARSER_RULE_DATA(GroupList, Alternatives(Idx<GroupListFields_Mailboxes>(), MailboxList(), Idx<GroupListFields_Comment>(), CFWS()))

// group           =   display-name ":" [group-list] ";" [CFWS]
PARSER_RULE_DATA(Group, Sequence(
    Idx<GroupFields_DisplayName>(), DisplayName(), Idx<INDEX_NONE>(), CharExact(':'), Idx<GroupFields_GroupList>(), GroupList(), Idx<INDEX_NONE>(), CharExact(';'), Idx<GroupFields_Comment>(), Optional(CFWS())))

// address         =   mailbox / group
PARSER_RULE_DATA(Address, Alternatives(Idx<AddressFields_Mailbox>(), Mailbox(), Idx<AddressFields_Group>(), Group()))

// address-list    =   (address *("," address)) / obs-addr-list
PARSER_RULE_DATA(AddressList, Sequence(Idx<INDEX_THIS>(), Head(Address()), Idx<INDEX_THIS>(),
    Repeat(Sequence(Idx<INDEX_NONE>(), CharExact(','), Idx<INDEX_THIS>(), Address()))))
}
