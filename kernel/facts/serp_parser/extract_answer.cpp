#include "extract_answer.h"
#include <util/string/builder.h>
#include <util/string/vector.h>

namespace NFactsSerpParser {

    static TString JoinChildrenText(
            const TStringBuf& separator, const NSc::TArray& node, const TStringBuf& textChildName = TStringBuf())
    {
        TStringBuilder result;

        for (const NSc::TValue& child: node) {
            if (result) {
                result << separator;
            }

            // Using ForceString, because table facts may have raw numbers in JSON.
            if (textChildName && child.IsDict()) {
                result << child[textChildName].ForceString();
            } else {
                result << child.ForceString();
            }
        }

        return result;
    }

    static TString GetObjectFactAnswer(const NSc::TValue& serpData)
    {
        return JoinChildrenText(", ", serpData["requested_facts"]["item"][0]["value"].GetArray(), "text");
    }

    static TString GetCaloriesAnswer(const NSc::TValue& serpData)
    {
        return TString{serpData["data"]["calories1"].GetString()};
    }

    static TString GetPoetryAnswer(const NSc::TValue& serpData)
    {
        return TString{serpData["docs"][0]["title"].GetString()};
    }

    static TString GetWordCardAnswer(const NSc::TValue& serpData)
    {
        return JoinChildrenText("; ", serpData["text"].GetArray(), "definition");
    }

    static TString GetMathAnswer(const NSc::TValue& serpData)
    {
        return TString{serpData["kind"].GetString()};
    }

    static TString GetTableAnswer(const NSc::TValue& serpData)
    {
        const NSc::TValue& data = serpData["data"];

        const NSc::TValue& tableHeadline = data["table_headline"];
        const NSc::TValue& header = data["header"];
        const NSc::TValue& rows = data["rows"];

        TVector<TString> items;
        items.reserve(rows.GetArray().size() + 2);

        if (tableHeadline.IsString()) {
            items.push_back(TString{tableHeadline.GetString()});
        }
        if (header.IsArray()) {
            items.push_back(JoinChildrenText(", ", header.GetArray()));
        }
        for (const NSc::TValue& row: rows.GetArray()) {
            items.push_back(JoinChildrenText(", ", row.GetArray(), "text"));
        }

        return JoinStrings(items, "; ");
    }

    static TString GetListAnswer(const NSc::TArray& serpDataText)
    {
        return JoinChildrenText(", ", serpDataText);
    }

    static TString GetDistanceAnswer(const NSc::TValue& serpData)
    {
        float dist = serpData["data"]["distance"].GetNumber();
        return ToString(static_cast<int>(dist / 1000.0)) + " км";
    }

    static TString GetRichFactAnswer(const NSc::TValue& serpData)
    {
        if (serpData.Has("text") && serpData["text"].IsString()) {
            return TString(serpData["text"].GetString());
        }
        TVector<TString> answerItems;
        for (const auto& item : serpData["visible_items"].GetArray()) {
            if (!item.Has("content")) {
                continue;
            }
            for (const auto& contentItem : item["content"].GetArray()) {
                if (contentItem["text"].GetString().empty()) {
                    continue;
                }
                answerItems.push_back(TString(contentItem["text"].GetString()));
            }
        }
        return JoinStrings(answerItems, " ");
    }

    static TString GetSuggestFactAnswer(const NSc::TValue& serpData) {
        TVector<TString> answerTexts;

        const NSc::TValue& text = serpData["text"];
        if (text.IsString()) {
            answerTexts.push_back(ToString(text.GetString()));
        }
        if (text.IsArray()) {
            answerTexts.push_back(GetListAnswer(text.GetArray()));
        }

        if (serpData.Has("list_items") && serpData["list_items"].IsArray()) {
            answerTexts.push_back(GetListAnswer(serpData["list_items"].GetArray()));
        }

        return JoinStrings(answerTexts, "; ");
    }

    TString ExtractAnswer(const NSc::TValue& serpData)
    {
        const TStringBuf& type = serpData["type"].GetString();

        if (type == "entity-fact") {
            return GetObjectFactAnswer(serpData);
        }
        if (type == "calories_fact") {
            return GetCaloriesAnswer(serpData);
        }
        if (type == "poetry_lover") {
            return GetPoetryAnswer(serpData);
        }
        if (type == "dict_fact") {
            return GetWordCardAnswer(serpData);
        }
        if (type == "table_fact") {
            return GetTableAnswer(serpData);
        }
        if (type == "math") {
            return GetMathAnswer(serpData);
        }
        if (type == "distance_fact") {
            return GetDistanceAnswer(serpData);
        }
        if (type == "rich_fact") {
            return GetRichFactAnswer(serpData);
        }
        if (type == "suggest_fact") {
            return GetSuggestFactAnswer(serpData);
        }

        const NSc::TValue& text = serpData["text"];
        if (text.IsString()) {
            return TString{text.GetString()};
        }
        if (text.IsArray()) {
            return GetListAnswer(text.GetArray());
        }
        // znatoki + fact_instruction
        if (text.IsDict() && text.Has("text") && text.Get("text").IsArray()) {
            return GetListAnswer(text.Get("text").GetArray());
        }

        return TString();
    }

} // namespace NFactsSerpParser
