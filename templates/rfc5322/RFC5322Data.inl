// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234
//
// data definitions for the email address parsing
#pragma once

#include "ParserBase.hpp"

NAMEDTUPLE_BEGIN(TextWithCommData)
    NAMEDTUPLE_ITEM(SubstringPos, CommentBefore, )
    NAMEDTUPLE_ITEM(SubstringPos, Content, )
    NAMEDTUPLE_ITEM(SubstringPos, CommentAfter, )
NAMEDTUPLE_END(TextWithCommData)

using MultiTextWithCommData = std::vector<TextWithCommData>;

NAMEDTUPLE_BEGIN(AddrSpecData)
    NAMEDTUPLE_ITEM(TextWithCommData, LocalPart, )
    NAMEDTUPLE_ITEM(TextWithCommData, DomainPart, )
NAMEDTUPLE_END(AddrSpecData)

NAMEDTUPLE_BEGIN(AngleAddrData)
    NAMEDTUPLE_ITEM(SubstringPos, CommentBefore, )
    NAMEDTUPLE_ITEM(AddrSpecData, Content, )
    NAMEDTUPLE_ITEM(SubstringPos, CommentAfter, )
NAMEDTUPLE_END(AngleAddrData)

NAMEDTUPLE_BEGIN(NameAddrData)
    NAMEDTUPLE_ITEM(MultiTextWithCommData, DisplayName, )
    NAMEDTUPLE_ITEM(AngleAddrData, Address, )
NAMEDTUPLE_END(NameAddrData)

NAMEDTUPLE_BEGIN(MailboxData)
    NAMEDTUPLE_ITEM(NameAddrData, NameAddr, )
    NAMEDTUPLE_ITEM(AddrSpecData, AddrSpec, )
NAMEDTUPLE_END(MailboxData)

using MailboxListData = std::vector<MailboxData>;

NAMEDTUPLE_BEGIN(GroupListData)
    NAMEDTUPLE_ITEM(MailboxListData, Mailboxes, )
    NAMEDTUPLE_ITEM(SubstringPos, Comment, )
NAMEDTUPLE_END(GroupListData)

NAMEDTUPLE_BEGIN(GroupData)
    NAMEDTUPLE_ITEM(MultiTextWithCommData, DisplayName, )
    NAMEDTUPLE_ITEM(GroupListData, GroupList, )
    NAMEDTUPLE_ITEM(SubstringPos, Comment, )
NAMEDTUPLE_END(GroupData)

NAMEDTUPLE_BEGIN(AddressData)
    NAMEDTUPLE_ITEM(MailboxData, Mailbox, )
    NAMEDTUPLE_ITEM(GroupData, Group, )
NAMEDTUPLE_END(AddressData)

using AddressListData = std::vector<AddressData>;

template <typename CHAR_TYPE>
std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, bool withComments, TextWithCommData const & text)
{
    std::basic_string<CHAR_TYPE> result;

    if (withComments)
    {
        result += ToString(buffer, SubstringPos(text.CommentBefore.first, text.CommentAfter.second));
    }
    else
    {
        result += ToString(buffer, text.Content);
    }

    return result;
}

template <typename CHAR_TYPE>
std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, bool withComments, MultiTextWithCommData const & text)
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
