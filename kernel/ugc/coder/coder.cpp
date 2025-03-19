#include "coder.h"

#include <kernel/ugc/aggregation/feedback.h>

namespace NUgc {
    namespace NCoder {
        static NSc::TValue GetRatingTemplate(const TString& table) {
            return NSc::TValue::FromJsonThrow(TStringBuilder() << R"(
            {
                "set":[
                    {
                        "table": ")" << table << R"(",
                        "options": [
                            "",
                            "1",
                            "2",
                            "3",
                            "4",
                            "5"
                        ],
                        "name": "ratingOverall",
                        "param": "rating-overall"
                    },
                    {
                        "table": ")" << table << R"(",
                        "name": "otype"
                    }
                ]
            })");
        }

        static NSc::TValue GetNotInterestedTemplate(const TString& table) {
            return NSc::TValue::FromJsonThrow(TStringBuilder() << R"(
            {
                "set":[
                    {
                        "table": ")" << table << R"(",
                        "options": ["yes", "no", ""],
                        "name": "notInterested",
                        "param": "not-interested"
                    },
                    {
                        "table": ")" << table << R"(",
                        "name": "otype"
                    },
                    {
                        "table": ")" << table << R"(",
                        "name": "ratingOverall",
                        "value": ""
                    }
                ]
            })");
        }

        static const TStringBuf ENCODED = "encoded";

        static TString Encode(const TString& in, const NSignUrl::TKey& key) {
            if (in.size() > MAXSTRLEN)
                ythrow yexception() << TStringBuf("Encode error: size of the input string is bigger than ") << MAXSTRLEN;
            return NSignUrl::Crypt(in, key);
        }

        static TString Decode(const TString& in, const NSignUrl::TKey& key) {
            if (in.size() > MAXSTRLEN)
                ythrow yexception() << TStringBuf("Decode error: size of the input string is bigger than ") << MAXSTRLEN;
            TString data(NSignUrl::Base64PadDecode(in));
            TString out;
            out.resize(NSignUrl::CryptedSize(data.size()));
            out.resize(NSignUrl::Decrypt(data.data(), data.size(), out.begin(), key.second));
            return out;
        }

        void Encode(NSc::TValue& val, const NSignUrl::TKey& key) {
            if (!val.IsDict())
                ythrow yexception() << TStringBuf("Encode error: input value must be a dictionary");

            val.Delete(ENCODED);
            TString encryptedJson = Encode(val.ToJson(), key);
            val.ClearDict();
            val[ENCODED] = encryptedJson;
        }

        void Decode(NSc::TValue& val, const NSignUrl::TKey& key) {
            if (!val.IsDict())
                ythrow yexception() << TStringBuf("Decode error: input value must be a dictionary");

            if (!val.Has(ENCODED))
                ythrow yexception() << "Decode error: field \"encoded\" is absent in the input"sv;

            TString decodedJson = Decode(TString(val[ENCODED].GetString()), key);
            val = NSc::TValue::FromJson(decodedJson);
        }

        void AddValueToResponse(
            const TRatingInfoParams& pr,
            const NSc::TValue& updateTemplate,
            const TStringBuf ugcdbField,
            const TStringBuf responseField,
            NSc::TValue response)
        {
            NSc::TValue js = updateTemplate;
            js["appId"] = pr.AppId;
            js["userId"] = pr.UserId;
            js["visitorId"] = pr.VisitorId;
            js["contextId"] = pr.ContextId;
            for (size_t i = 0; i < js["set"].GetArray().size(); ++i) {
                js["set"][i]["key"] = pr.ObjectKey;
                if (js["set"][i]["name"] == "otype") {
                    js["set"][i]["value"] = pr.Otype;
                }
            }

            NCoder::Encode(js, pr.Key);
            TString tokenField = TString::Join(responseField, "_token");
            response[tokenField] = TString(js["encoded"].GetString());

            TMaybe<TString> value;

            if (pr.Objects) {
                if (pr.Objects->contains(pr.ObjectKey)) {
                    const auto& object = pr.Objects->at(pr.ObjectKey);
                    if (responseField == "rating_overall" && object.HasRatingOverall()) {
                        value = ToString(object.GetRatingOverall().value() / 2);
                    } else if (responseField == "not_interested" && object.HasNotInterested()) {
                        value = object.GetNotInterested().value() ? "yes" : "no";
                    } else if (responseField == "rating_overall_10" && object.HasRatingOverall()) {
                        value = ToString(object.GetRatingOverall().value());
                    }
                }
            } else if (pr.Feedback) {
                if (pr.Otype == "Text/Book@on") {
                    value = pr.Feedback->Value("books", pr.ObjectKey, ugcdbField);
                } else if (pr.Otype == "Band/MusicGroup@on") {
                    value = pr.Feedback->Value("music-groups", pr.ObjectKey, ugcdbField);
                } else if (pr.Otype == "Music/Album@on") {
                    value = pr.Feedback->Value("music-albums", pr.ObjectKey, ugcdbField);
                } else {
                    value = pr.Feedback->Value("objects", pr.ObjectKey, ugcdbField);
                }

                if (responseField == "rating_overall_10" && value.Defined() && !value->empty()) {
                    value = ToString(FromString<float>(value.GetRef()) * 2);
                }
            }
            if (value && !value->empty()) {
                response[responseField] = std::move(*value);
            }
        }

        NSc::TValue GetRatingInfo(const TRatingInfoParams& pr) {
            if (!pr.IsValid()) {
                return NSc::TValue();
            }

            NSc::TValue res;
            try {
                TString table = "objects";
                if (pr.Otype == "Text/Book@on") {
                    table = "books";
                }
                if (pr.Otype == "Band/MusicGroup@on") {
                    table = "music-groups";
                }
                if (pr.Otype == "Music/Album@on") {
                    table = "music-albums";
                }
                AddValueToResponse(pr, GetRatingTemplate(table), "ratingOverall", "rating_overall", res);
                AddValueToResponse(pr, GetRatingTemplate(table), "ratingOverall", "rating_overall_10", res);
                AddValueToResponse(pr, GetNotInterestedTemplate(table), "notInterested", "not_interested", res);

                res["token"] = res["rating_overall_token"]; // for backward compatibility
            } catch (...) {
                return NSc::TValue();
            }
            return res;
        }
    } // namespace NCoder
} // namespace NUgc
