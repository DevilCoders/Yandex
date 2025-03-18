#include "parstypes.h"
#include "propface.h"

#include <library/cpp/html/face/propface.pb.h>
#include <library/cpp/containers/str_hash/str_hash.h>

#include <library/cpp/charset/ci_string.h>
#include <util/generic/hash.h>

namespace {
    class TParsedDocProperties: public IParsedDocProperties {
    public:
        TParsedDocProperties() {
            Charset = CODES_UNKNOWN;
        }

        int EnumerateValues(const char* name, std::function<void(const char*)> cb) const override {
            const auto pi = Properties_.find(name);

            if (pi == Properties_.end()) {
                return 1;
            }

            for (const auto& vi : pi->second) {
                cb(vi.c_str());
            }

            return 0;
        }

        int GetProperty(const char* name, const char** prop) const override {
            if (stricmp(name, PP_LINKS_HASH) == 0) {
                *prop = (const char*)&Links_;
                return 0;
            }
            const auto pi = Properties_.find(name);
            if (pi == Properties_.end())
                return 1;
            *prop = pi->second.back().c_str();
            return 0;
        }

        TStringBuf GetValueAtIndex(const TStringBuf name, size_t index) const override {
            const auto pi = Properties_.find(name);
            if (pi == Properties_.end()) {
                return {};
            }
            const auto& values = pi->second;
            return index < values.size() ? TStringBuf(values[index]) : TStringBuf();
        }

        bool HasProperty(const char* name) const override {
            return Properties_.find(name) != Properties_.end();
        }

        void Load(const TStringBuf& data) override {
            NHtml::TDocumentProperties result;

            if (!result.ParseFromArray(data.data(), data.size())) {
                ythrow yexception() << "can't read properties";
            }

            Properties_.clear();
            Links_.clear();

            for (const auto& pi : result.properties()) {
                for (auto vi = pi.values().begin(); vi != pi.values().end(); ++vi) {
                    this->SetProperty(pi.GetName().c_str(), vi->c_str());
                }
            }
            for (const auto& li : result.links()) {
                Links_.AddUniq(li.c_str());
            }
        }

        TBuffer Serialize() const override {
            NHtml::TDocumentProperties result;

            for (const auto& propertie : Properties_) {
                auto property = result.AddProperties();

                property->SetName(propertie.first);
                for (auto vi = propertie.second.begin(); vi != propertie.second.end(); ++vi) {
                    property->AddValues(*vi);
                }
            }
            for (const auto& link : Links_) {
                result.AddLinks(link.first);
            }

            const size_t size = result.ByteSize();
            TBuffer tmp(size);
            result.SerializeWithCachedSizesToArray((ui8*)tmp.Data());
            tmp.Advance(size);

            return tmp;
        }

        int SetProperty(const char* name, const char* prop) override {
            if (stricmp(name, PP_BASE) == 0) {
                THttpURL base;
                if (base.Parse(prop, THttpURL::FeaturesRobot) != THttpURL::ParsedOK)
                    return 1;
                if (!base.IsValidAbs())
                    return 1;
                base.FldClr(NUri::TField::FieldFrag);
                BaseUrl = base;
                InsertProperty(name, BaseUrl.PrintS());
                return 0;
            }
            if (stricmp(name, PP_CHARSET) == 0) {
                const ECharset enc = CharsetByName(prop);
                if (enc == CODES_UNKNOWN)
                    return 1;
                Charset = enc;
            } else if (stricmp(name, PP_ROBOTS) == 0) {
                if (strlen(prop) != 5)
                    return 1;
            } else if (stricmp(name, PP_LINKS_HASH) == 0) {
                if (!prop)
                    return 1;
                Links_.AddUniq(prop);
                return 0;
            }

            InsertProperty(name, prop);
            return 0;
        }

    private:
        void InsertProperty(const TCiString& name, const TString& value) {
            auto pi = Properties_.find(name);

            if (pi == Properties_.end()) {
                pi = Properties_.insert(std::make_pair(name, TVector<TString>())).first;
            }

            pi->second.push_back(value);
        }

    private:
        using TPropertyHash = THashMap<TCiString, TVector<TString>>;

        TPropertyHash Properties_;
        HashSet Links_;
    };

}

THolder<IParsedDocProperties> CreateParsedDocProperties() {
    return MakeHolder<TParsedDocProperties>();
}

const char* GetUrl(IParsedDocProperties* docProps) {
    const char* url = nullptr;
    docProps->GetProperty(PP_BASE, &url);
    return url;
}
