#include "secret_manager.h"
#include <library/cpp/string_utils/base64/base64.h>

namespace NUgc {
    namespace NSecurity {
        const TString BASE64_ENVIRONMENT = "BASE64_ENCODE";

        TSecretManager::TSecretManager(const TString& deploymentMode, TStringBuf singleKey)
            : Mode(deploymentMode) {
            if (!IsDeploymentMode(deploymentMode)) {
                ythrow TApplicationException()
                    <<  "[" << deploymentMode << "] is not a deployment mode.";
            }
            if (!singleKey.empty()) {
                AddSingleKey(singleKey);
            }
        }

        void TSecretManager::AddSingleKey(TStringBuf key) {
            SetKeyCountMode(SingleKeyMode);

            TString keySecret;
            if (ModeUsesLiteralKey()) {
                keySecret = key;
            } else {
                keySecret = GetKeyFromEnv(key);
            }
            Keys.insert(Keys.begin(), keySecret);
        }

        void TSecretManager::AddDatedKey(TStringBuf key) {
            SetKeyCountMode(DatedKeyMode);

            TString keySecret;
            TInstant keyDate;
            auto keyParts = ParseKey(key);
            if (ModeUsesLiteralKey()) {
                keySecret = keyParts.first;
                keyDate = keyParts.second;
            } else {
                keySecret = GetKeyFromEnv(key);
                keyDate = keyParts.second;
            }
            InsertKey(keySecret, keyDate);
        }

        const TString& TSecretManager::GetKey() const {
            if(Keys.empty()) {
                ythrow TApplicationException()
                    << "You should add at least one key.";
            }
            if (KeyCountMode != SingleKeyMode) {
                ythrow TApplicationException()
                    << "Not single key mode.";
            }
            return Keys[0];
        }

        const TString& TSecretManager::GetKeyByDate(TInstant date) const {
            if(Keys.empty()) {
                ythrow TApplicationException()
                    << "You should add at least one key.";
            }
            if (KeyCountMode != DatedKeyMode) {
                ythrow TApplicationException()
                    << "Not dated key mode.";
            }
            size_t position = FindDatePostition(date);
            if (position >= Keys.size()) {
                ythrow TApplicationException()
                    << "There there are no keys for date " << date.ToStringUpToSeconds();
            }
            return Keys[position];
        }

        const std::vector<TString>& TSecretManager::GetKeys() const {
            if(Keys.empty()) {
                ythrow TApplicationException()
                    << "You should add at least one key.";
            }
            return Keys;
        }

        size_t TSecretManager::FindDatePostition(const TInstant& newDate) const {
            // Linear search is fine because we don't expect many keys.
            for (size_t ind = 0; ind < Dates.size(); ++ind) {
                if (Dates[ind] < newDate) {
                    return ind;
                }
            }
            return Dates.size();
        }

        void TSecretManager::InsertKey(const TString& key, const TInstant& date) {
            size_t position = FindDatePostition(date);
            Keys.insert(Keys.begin() + position, key);
            Dates.insert(Dates.begin() + position, date);
        }

        std::pair<TStringBuf, TInstant> TSecretManager::ParseKey(TStringBuf keyName) const {
            TStringBuf namePart, datePart;
            TInstant datePartInstant;
            keyName.RSplit('_', namePart, datePart);
            // parse time in format yyyy-mm-dd
            datePartInstant = TInstant::ParseIso8601Deprecated(datePart);
            return {namePart, datePartInstant};
        }

        TString TSecretManager::GetKeyFromEnv(TStringBuf envVariable) const {
            TString keyStr = GetEnv(static_cast<TString>(envVariable));
            if (keyStr.empty()) {
                ythrow TApplicationException()
                    << "There is no environment variable with name [" << envVariable << "].";
            }

            TString base64Encode = GetEnv(BASE64_ENVIRONMENT);
            if (!base64Encode.empty() && base64Encode == "1") {
                try {
                    return Base64Decode(keyStr);
                } catch (yexception &e) {
                    ythrow TApplicationException()
                        << "Invalid encoded key with name [" << envVariable << "] and key [" << keyStr << "].";
                }
            }

            return keyStr;
        }

        bool TSecretManager::ModeUsesLiteralKey() const {
            return (Mode == DEV);
        }

        void TSecretManager::SetKeyCountMode(EKeyCountMode currentAddType) {
            if (KeyCountMode == UndefinedKeyMode)
                KeyCountMode = currentAddType;
            else {
                if (KeyCountMode == SingleKeyMode) {
                    ythrow TApplicationException()
                        << "You can't add more than one key.";
                } else if (currentAddType != DatedKeyMode) {
                    ythrow TApplicationException()
                        << "You can add only dated keys.";
                }
            }
        }
    }
}
