#pragma once

#include <kernel/common_server/common/scheme.h>

#include <mapreduce/yt/interface/common.h>

class TYtProcessTraits {
public:
    using TSchema = TMap<TString, NYT::EValueType>;

    static NYT::TTableSchema GetYtSchema(const TSchema& schema);

protected:
    TYtProcessTraits() = default;

    NYT::TTableSchema GetYtSchema() const;
    bool HasYtSchema() const;

    NYT::TNode Schematize(NYT::TNode&& record, const NYT::TTableSchema& schema) const;
    NYT::TNode Schematize(NYT::TNode&& record) const;

    void FillScheme(NFrontend::TScheme& scheme) const;
    void Serialize(NJson::TJsonValue& value) const;
    Y_WARN_UNUSED_RESULT bool Deserialize(const NJson::TJsonValue& value);

private:
    TMap<TString, NYT::EValueType> Schema;

    NYT::TTableSchema YtSchema;
};
