#include "yt_dumper.h"
#include "object.h"

#include <kernel/common_server/rt_background/processes/dumper/meta_parser.h>
#include <kernel/common_server/library/storage/structured.h>

class TTagYTDumper: public IDumperMetaParser {
public:
    static TFactory::TRegistrator<TTagYTDumper> Registrator;

    virtual NJson::TJsonValue ParseMeta(const NCS::NStorage::TDBValue& /*data*/) const override {
        return NJson::JSON_NULL;
    }
    bool NeedFullRecord() const override {
        return true;
    }
    virtual NJson::TJsonValue ParseMeta(const NCS::NStorage::TTableRecordWT& tr) const override {
        TDBTag dbTag;
        if (!TBaseDecoder::DeserializeFromTableRecordStrictable(dbTag, tr, false)) {
            return NJson::JSON_NULL;
        }
        return dbTag.SerializeToJson();
    }
};

TTagYTDumper::TFactory::TRegistrator<TTagYTDumper> TTagYTDumper::Registrator("tag_object");
