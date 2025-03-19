#include "dump.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

#include <util/charset/wide.h>
#include <util/stream/mem.h>

bool NUrlMenu::Deserialize(TUrlMenuVector& urlMenu, const TString& savedObject) {
    using TJsonValue = NJson::TJsonValue;
    using TJsonArray = NJson::TJsonValue::TArray;

    TJsonValue obj;

    TMemoryInput mi(savedObject.data(), savedObject.size());
    if (!NJson::ReadJsonTree(&mi, &obj)) {
        return false;
    }

    urlMenu.clear();
    if (obj.IsArray()) {
        const TJsonArray& items = obj.GetArray();
        for (const TJsonValue& item : items) {
            if (!item.IsArray()) {
                continue;
            }
            const TJsonArray& fields = item.GetArray();
            if (fields.size() == 2 && fields[0].IsString() && fields[1].IsString()) {
                const TUtf16String url = UTF8ToWide(fields[0].GetString());
                const TUtf16String name = UTF8ToWide(fields[1].GetString());
                if (urlMenu.size() > 0 && urlMenu.back().second == name && !url.empty()) {
                    urlMenu.pop_back();
                }
                urlMenu.push_back(std::pair<TUtf16String, TUtf16String>(url, name));
            }
        }
    }
    return true;
}

NJson::TJsonValue NUrlMenu::SerializeToJsonValue(const TUrlMenuVector& urlMenu) {
    NJson::TJsonValue obj(NJson::JSON_ARRAY);
    for (const auto& menuItem : urlMenu) {
        NJson::TJsonValue item(NJson::JSON_ARRAY);
        item.AppendValue(WideToUTF8(menuItem.first));
        item.AppendValue(WideToUTF8(menuItem.second));
        obj.AppendValue(item);
    }
    return obj;
}

TString NUrlMenu::Serialize(const TUrlMenuVector& urlMenu) {
    return NJson::WriteJson(SerializeToJsonValue(urlMenu), false);
}

