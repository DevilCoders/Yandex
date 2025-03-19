#include "cntintrf.h"

void IArchiveDocInfo::SerializeFirstStageAttributes(TArrayRef<const char * const> attrNames, IAttributeWriter& write) const {
    for (const auto& attrName: attrNames) {
        SerializeFirstStageAttribute(attrName, write);
    }
}

void IArchiveDocInfo::SerializeAttributes(TArrayRef<const char * const> attrNames, IAttributeWriter& write) const {
    for (const auto& attrName: attrNames) {
        SerializeAttribute(attrName, write);
    }
}

void IArchiveDocInfo::SerializeFirstStageAttribute(const char *, IAttributeWriter&) const {
    Y_FAIL("Not implemented: either IArchiveDocInfo::SerializeFirstStageAttribute or IArchiveDocInfo::SerializeFirstStageAttributes must be overridden");
}

void IArchiveDocInfo::SerializeAttribute(const char *, IAttributeWriter&) const {
    Y_FAIL("Not implemented: either IArchiveDocInfo::SerializeAttribute or IArchiveDocInfo::SerializeAttributes must be overridden");
}

