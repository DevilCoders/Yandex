#include "objectid.h"

#include <util/generic/bt_exception.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/split.h>

namespace NUgc {
    namespace {
        void ValidateIdStructure(TStringBuf objectId) {
            TStringBuf id;

            if (objectId.AfterPrefix("/sprav/", id)) {
                ui64 permalink;
                Y_ENSURE_EX(TryFromString(id, permalink), TBadArgumentException());
                return;
            }

            // XXX: consider any other string is valid object identifier
            // because we can't check it without asking service
        }
    } // namespace

    const TVector<TString> TObjectId::ValidPrefixes = {
        "/sprav/",
        "/ontoid/"
    };

    TObjectId::TObjectId(TStringBuf objectId)
        : ObjectId(objectId)
    {
        try {
            Y_ENSURE_EX(!ObjectId.empty(), TBadArgumentException());
            if (!objectId.StartsWith('/')) {
                // XXX: consider all ids without prefix is ontoids
                ObjectId = TStringBuilder() << "/ontoid/" << objectId;
            }
            ValidateIdStructure(ObjectId);
        } catch (const TBadArgumentException&) {
            ythrow TBadArgumentException() << "invalid objectId " << objectId;
        }
    }

    const TString& TObjectId::AsString() const {
        return ObjectId;
    }

    TString TObjectId::AsUgcdbKey() const {
        if (ObjectId.StartsWith("/ontoid/")) {
            TStringBuf ontoid;
            TStringBuf(ObjectId).AfterPrefix("/ontoid/", ontoid);
            return ToString(ontoid);
        }
        return ObjectId;
    }

    TString TObjectId::AsUgcdb2Key() const {
        TVector<TStringBuf> parts = StringSplitter(ObjectId).Split('/');
        Y_ENSURE_EX(parts.size() > 0, TBadArgumentException());
        return ToString(parts.back());
    }
} // namespace NUgc

