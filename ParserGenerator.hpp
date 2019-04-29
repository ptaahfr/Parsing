// (c) 2019 ptaahfr http://github.com/ptaahfr
// All right reserved, for educational purposes
//
// ABNF parser generation
#pragma once

#include <vector>
#include <iostream>
#include <map>
#include <set>
#include <sstream>

#include "ParserRFC5234.hpp"

void GenerateABNFParser(std::ostream & os, RFC5234ABNF::RuleListData const & rules, std::vector<char> const & buffer)
{
    using namespace RFC5234ABNF;

    std::map<std::string, std::tuple<std::list<std::string>, std::list<std::string> > > mapRules;

    auto transformRulename = [&] (auto const & ruleNameField)
    {
        auto ruleName(ToString(buffer, ruleNameField));
        std::transform(ruleName.begin(), ruleName.end(), ruleName.begin(), [](char ch) { return ch == '-' ? '_' : ch; } );
        return ruleName;
    };

    for (auto const & rule : rules)
    {
        std::stringstream cout; // << "PARSER_RULE(" << transformRulename(std::get<RuleFields_Rulename>(rule)) << ", ";
        std::list<std::string> alternatives;
        std::list<std::string> dependencies;
        std::function<void(AlternationData const &)> generateAlternation;

        auto generateChar = [&](auto value, char const * prefix)
        {
            cout << "CharVal<" << prefix << ToString(buffer, value) << ">()";
        };

        auto generateNumValSpec = [&](NumValSpecData const & numValSpec, char const * prefix)
        {
            auto const & firstValue(std::get<NumValSpecFields_FirstValue>(numValSpec));
            auto const & seqValues(std::get<NumValSpecFields_SequenceValues>(numValSpec));
            auto const & rangeLastValue(std::get<NumValSpecFields_RangeLastValue>(numValSpec));

            if (!IsNull(seqValues))
            {
                cout << "Sequence(";
                generateChar(firstValue, prefix);
                for (auto const & seqValue : seqValues)
                {
                    cout << ", ";
                    generateChar(seqValue, prefix);
                }
                cout << ")";
            }
            else if (!IsNull(rangeLastValue))
            {
                cout << "CharRange<" << prefix << ToString(buffer, firstValue) << ", " << prefix << ToString(buffer, rangeLastValue) << ">()";
            }
            else
            {
                generateChar(firstValue, prefix);
            }
        };

        auto generateNumVal = [&](NumValData const & numVal)
        {
            auto const & hexVal(std::get<NumValFields_Hex>(numVal));
            if (!IsNull(hexVal))
            {
                generateNumValSpec(hexVal, "0x");
            }
            else
            {
                auto const & binVal(std::get<NumValFields_Bin>(numVal));
                if (!IsNull(binVal))
                {
                    generateNumValSpec(binVal, "0b");
                }
                else
                {
                    generateNumValSpec(std::get<NumValFields_Dec>(numVal), "");
                }
            }
        };

        auto generateElement = [&](ElementData const & element, bool insideRepeat)
        {
            auto charValOrProseVal(std::get<ElementFields_CharVal>(element));
            if (IsNull(charValOrProseVal))
                charValOrProseVal = std::get<ElementFields_ProseVal>(element);
            if (!IsNull(charValOrProseVal))
            {
                // Skip delimiters <> or ""
                size_t length(std::get<1>(charValOrProseVal) - std::get<0>(charValOrProseVal));
                if (length > 2)
                {
                    if (length > 3 && !insideRepeat)
                    {
                        cout << "Sequence(";
                    }

                    bool first = true;
                    for (size_t t = std::get<0>(charValOrProseVal) + 1; t < std::get<1>(charValOrProseVal) - 1; ++t)
                    {
                        if (!first)
                            cout << ", ";
                        first = false;

                        cout << "CharVal<0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(2) << (MaxCharType)buffer[t] << ">()";
                    }

                    if (length > 3 && !insideRepeat)
                    {
                        cout << ")";
                    }
                }
            }
            else
            {
                auto const & numVal(std::get<ElementFields_NumVal>(element));
                if (!IsNull(numVal))
                {
                    generateNumVal(numVal);
                }
                else
                {
                    auto const & group(std::get<ElementFields_Group>(element));
                    if (!IsNull(group))
                    {
                        generateAlternation(group);
                    }
                    else
                    {
                        auto const & option(std::get<ElementFields_Option>(element));
                        if (!IsNull(option))
                        {
                            cout << "Optional(";
                            generateAlternation(option);
                            cout << ")";
                        }
                        else
                        {
                            std::string otherRuleName(transformRulename(std::get<ElementFields_Rulename>(element)));
                            dependencies.push_back(otherRuleName);
                            cout << otherRuleName << "()";
                        }
                    }
                }
            }
        };

        auto generateRepetition = [&](RepetitionData const & repetition)
        {
            auto const & repeat(std::get<RepetitionFields_Repeat>(repetition));
            auto const & element(std::get<RepetitionFields_Element>(repetition));

            bool insideRepeat(!IsNull(repeat));

            if (insideRepeat)
            {
                cout << "Repeat";

                if (!IsEmpty(repeat))
                {
                    auto const & fixed(std::get<RepeatFields_Fixed>(repeat));
                    auto const & range(std::get<RepeatFields_Range>(repeat));

                    cout << "<";

                    if (!IsEmpty(fixed))
                    {
                        auto num(ToString(buffer, fixed));
                        cout << num << ", " << num << ">(";
                    }
                    else if (IsEmpty(std::get<0>(range)))
                    {
                        cout << "0, " << ToString(buffer, std::get<1>(range));
                    }
                    else if (IsEmpty(std::get<1>(range)))
                    {
                        cout << ToString(buffer, std::get<0>(range));
                    }
                    else
                    {
                        cout << ToString(buffer, std::get<0>(range)) << ", " << ToString(buffer, std::get<1>(range)) << ">(";
                    }

                    cout << ">";
                }

                cout << "(";
            }

            generateElement(element, insideRepeat);

            if (insideRepeat)
            {
                cout << ")";
            }
        };

        auto generateConcatenation = [&](ConcatenationData const & concatenation)
        {
            if (concatenation.size() == 1)
            {
                generateRepetition(concatenation.front());
            }
            else
            {
                bool first = true;
                cout << "Sequence(";
                for (auto const & repetition : concatenation)
                {
                    if (!first)
                        cout << ", ";
                    first = false;
                    generateRepetition(repetition);
                }
                cout << ")";
            }
            // cout << 
        };

        generateAlternation = [&](AlternationData const & alternation)
        {
            if (alternation.size() == 1)
            {
                generateConcatenation(alternation.front());
            }
            else
            {
                bool first = true;
                cout << "Alternatives(";
                for (auto const & concatenation : alternation)
                {
                    if (!first)
                        cout << ", ";
                    first = false;
                    generateConcatenation(concatenation);
                }
                cout << ")";
            }
        };

        for (auto const & concatenation : std::get<RuleFields_Elements>(rule))
        {
            generateConcatenation(concatenation);
            alternatives.push_back(cout.str());
            cout = std::stringstream();
        }

        std::string ruleName(transformRulename(std::get<RuleFields_Rulename>(rule)));
        std::string definedAs(ToString(buffer, std::get<RuleFields_DefinedAs>(rule)));
        
        if (definedAs.find("=/") != std::string::npos)
        {
            auto & ruleEntryPtr(mapRules.find(ruleName));
            if (ruleEntryPtr != mapRules.end())
            {
                std::get<0>(ruleEntryPtr->second).splice(std::get<0>(ruleEntryPtr->second).end(), dependencies);
                std::get<1>(ruleEntryPtr->second).splice(std::get<1>(ruleEntryPtr->second).end(), alternatives);
            }
        }
        else
        {
            mapRules[ruleName] = std::make_tuple(std::move(dependencies), std::move(alternatives));
        }
    }

    std::set<std::string> parsingRules, parsedRules, parsedRulesForward;
    std::function<void(std::string const &)> outputRule;
    outputRule = [&](std::string const & ruleName)
    {
        if (parsedRules.find(ruleName) != parsedRules.end())
            return;
        if (parsingRules.find(ruleName) != parsingRules.end())
        {
            if (parsedRulesForward.find(ruleName) == parsedRulesForward.end())
            {
                os << "PARSER_RULE_FORWARD(" << ruleName << ");" << std::endl << std::endl;
                parsedRulesForward.insert(ruleName);
            }
        }
        else
        {
            parsingRules.insert(ruleName);

            auto ruleEntryPtr = mapRules.find(ruleName);
            if (mapRules.end() != ruleEntryPtr)
            {
                for (auto const & dep : std::get<0>(ruleEntryPtr->second))
                {
                    outputRule(dep);
                }

                if (parsedRulesForward.find(ruleName) != parsedRulesForward.end())
                {
                    os << "PARSER_RULE_PARTIAL(";
                }
                else
                {
                    os << "PARSER_RULE(";
                }

                os << ruleName << ", ";
                
                if (std::get<1>(ruleEntryPtr->second).size() > 1)
                {
                    os << "Alternatives(";
                }

                bool first = true;
                for (auto const & alt : std::get<1>(ruleEntryPtr->second))
                {
                    if (!first)
                        os << ", ";
                    first = false;
                    os << alt;
                }

                if (std::get<1>(ruleEntryPtr->second).size() > 1)
                {
                    os << ")";
                }

                os << ");" << std::endl << std::endl;
            }
            parsedRules.insert(ruleName);
        }

    };
    for (auto const & rule : mapRules)
    {
        outputRule(rule.first);
    }
}
