#pragma once

#include "ParserBase.hpp"

using TextWithCommData = std::tuple<SubstringPos, SubstringPos, SubstringPos>; // CommentBefore, Content, CommentAfter
enum TextWithCommFields
{
    TextWithCommFields_CommentBefore,
    TextWithCommFields_Content,
    TextWithCommFields_CommentAfter
};

using MultiTextWithComm = std::vector<TextWithCommData>;

using AddrSpecData = std::tuple<TextWithCommData, TextWithCommData>; // LocalPart, DomainPart
enum AddrSpecFields
{
    AddrSpecFields_LocalPart,
    AddrSpecFields_DomainPart,
};

using AngleAddrData = std::tuple<SubstringPos, AddrSpecData, SubstringPos>; // CommentBefore, Content, CommentAfter
enum AngleAddrFields
{
    AngleAddrFields_CommentBefore,
    AngleAddrFields_Content,
    AngleAddrFields_CommentAfter
};

using NameAddrData = std::tuple<MultiTextWithComm, AngleAddrData>; // DisplayName, Address
enum NameAddrFields
{
    NameAddrFields_DisplayName,
    NameAddrFields_Address,
};

using MailboxData = std::tuple<NameAddrData, AddrSpecData>; // NameAddr, AddrSpec
enum MailboxFields
{
    MailboxFields_NameAddr,
    MailboxFields_AddrSpec,
};

using MailboxListData = std::vector<MailboxData>;

using GroupListData = std::tuple<MailboxListData, SubstringPos>; // Mailboxes, Comment
enum GroupListFields
{
    GroupListFields_Mailboxes,
    GroupListFields_Comment
};

using GroupData = std::tuple<MultiTextWithComm, GroupListData, SubstringPos>; // DisplayName, GroupList, Comment
enum GroupFields
{
    GroupFields_DisplayName,
    GroupFields_GroupList,
    GroupFields_Comment
};

using AddressData = std::tuple<MailboxData, GroupData>;
enum AddressFields
{
    AddressFields_Mailbox,
    AddressFields_Group
};
using AddressListData = std::vector<AddressData>;


template <typename CHAR_TYPE>
std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, bool withComments, TextWithCommData const & text)
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

