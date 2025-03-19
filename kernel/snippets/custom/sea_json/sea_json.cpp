#include "sea_json.h"

#include <kernel/snippets/idl/raw_preview.pb.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/json/json_writer.h>

namespace NSnippets {
    namespace {
        void PasteSeaJsonPropertyItems(NJson::TJsonValue& jsonMap, const NProto::TPreviewItem& previewItem) {
            NJson::TJsonValue jsonPropertyItems(NJson::JSON_ARRAY);
            NJson::TJsonValue jsonMainPropertyItems(NJson::JSON_ARRAY);
            for (const auto& property : previewItem.GetProperties()) {
                if (!(property.HasKey() && property.HasValue()))
                    continue;
                NJson::TJsonValue jsonKeyValueItems(NJson::JSON_ARRAY);
                jsonKeyValueItems.AppendValue(property.GetKey());
                jsonKeyValueItems.AppendValue(property.GetValue());
                if (property.HasIsMain() && property.GetIsMain())
                    jsonMainPropertyItems.AppendValue(jsonKeyValueItems);
                else
                    jsonPropertyItems.AppendValue(jsonKeyValueItems);
            }
            if (jsonPropertyItems.GetArraySafe().size() > 0)
                jsonMap.InsertValue("properties", jsonPropertyItems);
            if (jsonMainPropertyItems.GetArraySafe().size() > 0)
                jsonMap.InsertValue("main-properties", jsonMainPropertyItems);
        }

        void PastSeaJsonImageItems(NJson::TJsonValue& jsonMap, const NProto::TPreviewItem& previewItem) {
            NJson::TJsonValue jsonImageItems(NJson::JSON_ARRAY);
            for (const auto& image : previewItem.GetImages())
                if (image.HasUrl())
                    jsonImageItems.AppendValue(image.GetUrl());
            if (jsonImageItems.GetArraySafe().size() == 1)
                jsonMap.InsertValue("image", jsonImageItems.GetArraySafe()[0]);
            else if (jsonImageItems.GetArraySafe().size() > 1)
                jsonMap.InsertValue("images", jsonImageItems);
        }

        void FillSeaJsonResultItem(NJson::TJsonValue& jsonMap, const NProto::TPreviewItem& previewItem) {
            if (previewItem.HasName())
                jsonMap.InsertValue("name", previewItem.GetName());
            if (previewItem.HasDescription())
                jsonMap.InsertValue("description", previewItem.GetDescription());
            if (previewItem.HasUrl())
                jsonMap.InsertValue("url", previewItem.GetUrl());
            if (previewItem.ImagesSize() > 0)
                PastSeaJsonImageItems(jsonMap, previewItem);
            if (previewItem.PropertiesSize() > 0)
                PasteSeaJsonPropertyItems(jsonMap, previewItem);
        }
    }

    TString MakeSeaJson(const NProto::TRawPreview& rawPreview) {
        NJson::TJsonValue resultJson(NJson::JSON_MAP);
        if (rawPreview.HasTemplate())
            resultJson.InsertValue("template", rawPreview.GetTemplate());
        if (rawPreview.HasResultsCountInAll()) {
            resultJson.InsertValue("results_count", rawPreview.GetResultsCountInAll());
            NJson::TJsonValue jsonResultItems(NJson::JSON_ARRAY);
            for (const auto& previewItem : rawPreview.GetPreviewItems()) {
                NJson::TJsonValue jsonResultItem(NJson::JSON_MAP);
                FillSeaJsonResultItem(jsonResultItem, previewItem);
                if (jsonResultItem.GetMapSafe().size() > 0)
                    jsonResultItems.AppendValue(jsonResultItem);
            }
            if (jsonResultItems.GetArraySafe().size() > 0)
                resultJson.InsertValue("results", jsonResultItems);
        } else {
            FillSeaJsonResultItem(resultJson, rawPreview.GetPreviewItems(0));
        }
        TString resultString;
        TStringOutput stringOutput(resultString);
        NJson::TJsonWriter jsonWriter(&stringOutput, true);
        jsonWriter.Write(&resultJson);
        jsonWriter.Flush();
        return resultString;
    }
}
