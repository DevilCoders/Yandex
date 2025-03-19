#include "meta_parser.h"

bool IDumperMetaParser::NeedFullRecord() const {
    return false;
}

NJson::TJsonValue IDumperMetaParser::ParseMeta(const NCS::NStorage::TTableRecordWT& /*tr*/) const {
    return NJson::JSON_NULL;
}
