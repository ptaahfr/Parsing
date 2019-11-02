// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// test parsing code for email adresses based on RFC 5322 & 5234
//
// parser IO and base types
#pragma once

#include <utility>
#include <string>
#include <cstdint>
#include <vector>
#include <cassert>
#include <algorithm>

#define ENABLED_IF(condition) std::enable_if_t<(condition), void *> = nullptr
#define ENABLED_IF_DEF(condition) std::enable_if_t<(condition), void *>
#define ENABLED_IF_RET(condition, return_type) std::enable_if_t<(condition), return_type>
#define CONSTANT(...) std::remove_reference_t<decltype(__VA_ARGS__)>::value
#define CONSTANT_F(funcname, ...) std::remove_reference_t<decltype(funcname(std::declval<__VA_ARGS__>()))>::value
#define TYPE_F(funcname, ...) typename std::remove_reference_t<decltype(funcname(std::declval<__VA_ARGS__>()))>

using MaxCharType = int;

using SubstringPos = std::pair<size_t, size_t>;

template <typename CHAR_TYPE>
inline std::basic_string<CHAR_TYPE> ToString(std::vector<CHAR_TYPE> const & buffer, SubstringPos subString)
{
    subString.first = std::min(subString.first, buffer.size());
    subString.second = std::max(subString.first, std::min(subString.second, buffer.size()));
    return std::basic_string<CHAR_TYPE>(buffer.data() + subString.first, buffer.data() + subString.second);
}

inline bool IsEmpty(SubstringPos const & sub)
{
    return sub.second <= sub.first;
}

inline bool IsEmpty(nullptr_t)
{
    return true;
}

template <typename ELEM>
inline bool IsEmpty(std::vector<ELEM> const & arr);

template <size_t OFFSET, typename... ARGS>
inline bool IsEmpty(std::tuple<ARGS...> const & tuple);

template <typename... ARGS>
inline bool IsEmpty(std::tuple<ARGS...> const & tuple)
{
    return IsEmpty<0>(tuple);
}

template <size_t OFFSET, typename... ARGS>
inline bool IsEmpty(std::tuple<ARGS...> const & tuple)
{
    enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(ARGS) - 1) };
    if (false == IsEmpty(std::get<OFFSET>(tuple)))
        return false;
    if (NEXT_OFFSET > OFFSET)
        return IsEmpty<NEXT_OFFSET>(tuple);
    return true;
}

template <typename ELEM>
inline bool IsEmpty(std::vector<ELEM> const & arr)
{
    for (auto const & elem : arr)
    {
        if (false == IsEmpty(elem))
            return false;
    }
    return true;
}

inline bool IsNull(SubstringPos const & sub)
{
    return IsEmpty(sub) && sub.first == 0;
}

inline bool IsNull(nullptr_t)
{
    return true;
}

template <typename ELEM>
inline bool IsNull(std::vector<ELEM> const & arr);

template <size_t OFFSET, typename... ARGS>
inline bool IsNull(std::tuple<ARGS...> const & tuple);

template <typename... ARGS>
inline bool IsNull(std::tuple<ARGS...> const & tuple)
{
    return IsNull<0>(tuple);
}

template <size_t OFFSET, typename... ARGS>
inline bool IsNull(std::tuple<ARGS...> const & tuple)
{
    enum { NEXT_OFFSET = __min(OFFSET + 1, sizeof...(ARGS) - 1) };
    if (false == IsNull(std::get<OFFSET>(tuple)))
        return false;
    if (NEXT_OFFSET > OFFSET)
        return IsNull<NEXT_OFFSET>(tuple);
    return true;
}

template <typename ELEM>
inline bool IsNull(std::vector<ELEM> const & arr)
{
    for (auto const & elem : arr)
    {
        if (false == IsNull(elem))
            return false;
    }
    return true;
}
