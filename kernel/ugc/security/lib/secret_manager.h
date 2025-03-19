#pragma once

#include <vector>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/system/env.h>
#include <kernel/ugc/runtime/modes.h>

#include "exception.h"

namespace NUgc {
    namespace NSecurity {
        class TSecretManager{
        public:
            explicit TSecretManager(const TString& deploymentMode, TStringBuf singleKey = "");
            void AddSingleKey(TStringBuf key);  // env variable name for prod and test, the key value otherwise
            void AddDatedKey(TStringBuf key); // same as above with _yyyy-mm-dd suffix
            const TString& GetKey() const; // currently effective key
            const TString& GetKeyByDate(TInstant date) const;
            const std::vector<TString>& GetKeys() const;

        private:
            size_t FindDatePostition(const TInstant& newDate) const;
            void InsertKey(const TString&  key, const TInstant& date);
            std::pair<TStringBuf, TInstant> ParseKey(TStringBuf keyName) const;
            TString GetKeyFromEnv(TStringBuf envVariable) const;
            bool ModeUsesLiteralKey() const;

            enum EKeyCountMode {
                UndefinedKeyMode,
                SingleKeyMode,
                DatedKeyMode
            };
            void SetKeyCountMode(EKeyCountMode currentAddType);

        private:
            TString Mode;
            EKeyCountMode KeyCountMode = UndefinedKeyMode;
            std::vector<TString> Keys;
            std::vector<TInstant> Dates;
        };
    } //namespace NUgc
} // namespace NSecurity
