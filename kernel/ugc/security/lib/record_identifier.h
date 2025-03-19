#pragma once

#include <kernel/ugc/security/proto/record_identifier_bundle.pb.h>

namespace NUgc {
    namespace NSecurity {
        // Generates string identifiers for UGC records. Expectations:
        // - Multiple attempts to record the same data generate the same identifier.
        // - Different data produces different identifiers (with extremely high probability).
        // - Intentional collisions are impractical, especially with someone else's records.
        // - Recovering original data from identifiers is impractical.
        //
        // The identifiers are ASCII strings. They are suitable for direct use in URLs, filenames or
        // as database keys, and are intended to be used as-is. The length of identifiers is
        // unspecified, but reasonably short.
        TString GenerateRecordIdentifierByHash(const TRecordIdentifierBundle& bundle);

        class TRecordIdentifierGenerator {
            TRecordIdentifierBundle bundle;

        public:
            TRecordIdentifierGenerator(const TString& ns, const TString& userId) {
                bundle.SetNamespace(ns);
                bundle.SetUserId(userId);
            }

            TRecordIdentifierGenerator& ContextId(const TString& contextId) {
                bundle.SetContextId(contextId);
                return *this;
            }

            TRecordIdentifierGenerator& ParentId(const TString& parentId) {
                bundle.SetParentId(parentId);
                return *this;
            }

            TRecordIdentifierGenerator& ClientEntropy(const TString& clientEntropy) {
                bundle.SetClientEntropy(clientEntropy);
                return *this;
            }

            TString Build();
        };
    } // namespace NSecurity
} // namespace NUgc
