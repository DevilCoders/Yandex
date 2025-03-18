// ====================================================================
//  Implementation of blackbox response options
// ====================================================================

#include "responseimpl.h"
#include "utils.h"

#include <library/cpp/blackbox2/options.h>

namespace NBlackbox2 {
    /// Construct options list with one option inside
    TOptions::TOptions(const TOption& opt)
        : Options_(1, opt)
    {
    }

    /// Add option to the list as stream
    TOptions& TOptions::operator<<(const TOption& opt) {
        Options_.push_back(opt);
        return *this;
    }

    /// Transform email option to options list
    TOptions& TOptTestEmail::Format(TOptions& opts) const {
        opts << TOption("emails", "testone");
        TString punycode;
        const TString& ref = NIdn::EncodeAddr(AddrToTest_, punycode);
        opts << TOption("addrtotest", ref);

        return opts;
    }

    TDBFields::TDBFields(const TString& field) {
        // add one field key
        if (!field.empty())
            Fields_[field];
    }

    TDBFields::TDBFields(const TResponse* resp) {
        ReadResponse(resp);
    }

    void TDBFields::ReadResponse(const TResponse* resp) {
        if (!resp)
            throw TFatalError(NSessionCodes::INVALID_PARAMS, "NULL Response pointer");

        // need to clean old values first
        ClearValues();

        // Then add all fields coming from Response
        TResponse::TImpl* pImpl = resp->GetImpl();

        xmlConfig::Parts fields(pImpl->GetParts("dbfield"));
        TString id, val;

        for (int i = 0; i < fields.Size(); ++i) {
            xmlConfig::Part item(fields[i]);
            if (!item.GetIfExists("@id", id))
                throw TFatalError(NSessionCodes::UNKNOWN,
                                  "Bad blackbox response: missing dbfield/@id");

            if (id.find("login") != id.npos ||
                id.find("email") != id.npos) {
                TString tmp;
                TString itemString = item.asString();
                const TString& ref = NIdn::DecodeAddr(itemString, tmp);
                val = ref;
            } else
                val = item.asString();

            Fields_[id] = val;
        }
    }

    void TDBFields::Clear() {
        Fields_.clear();
    }

    void TDBFields::ClearValues() {
        TContainerType::iterator p = Fields_.begin();

        while (p != Fields_.end())
            p++->second.clear();
    }

    TDBFields& TDBFields::operator<<(const TString& field) {
        // adds the key if does not exist
        if (!field.empty())
            Fields_[field];
        return *this;
    }

    const TString& TDBFields::Get(const TString& field) const {
        auto it = Fields_.find(field);

        if (it != Fields_.end())
            return it->second;
        else
            return EMPTY_STR;
    }

    bool TDBFields::Has(const TString& field) const {
        return End() != Fields_.find(field);
    }

    TDBFields::TContainerType::const_iterator TDBFields::Begin() const {
        return Fields_.begin();
    }

    TDBFields::TContainerType::const_iterator TDBFields::End() const {
        return Fields_.end();
    }

    TDBFields::operator TOptions() const {
        TOptions r;
        return r << *this;
    }

    TOptions& operator<<(TOptions& options, const TDBFields& opt) {
        TString fields_list;

        if (opt.Empty())
            return options; // don't write if empty

        auto it = opt.Begin();

        fields_list.append(it++->first);

        while (it != opt.End()) {
            fields_list.append(1, ',');
            fields_list.append(it++->first);
        }

        return options << TOption("dbfields", fields_list);
    }

    TOptions& operator<<(TOptions& options, const TOptTestEmail& opt) {
        return opt.Format(options);
    }

    TOptAliases::TOptAliases(const TString& alias) {
        if (!alias.empty())
            Aliases_.insert(alias);
    }

    TOptAliases& TOptAliases::operator<<(const TString& alias) {
        if (!alias.empty())
            Aliases_.insert(alias);
        return *this;
    }

    TOptAliases::operator TOptions() const {
        TOptions r;
        return r << *this;
    }

    TOption TOptAliases::Format() const {
        TString aliases_list;

        auto it = Aliases_.begin();

        if (it != Aliases_.end())
            aliases_list.append(*it++);

        while (it != Aliases_.end()) {
            aliases_list.append(1, ',');
            aliases_list.append(*it++);
        }

        return TOption("aliases", aliases_list);
    }

    TOptions& operator<<(TOptions& options, const TOptAliases& opt) {
        if (!opt.Empty())
            options << opt.Format();
        return options;
    }

    TAttributes::TAttributes(const TString& attr) {
        // add one field key
        if (!attr.empty())
            Attributes_[attr];
    }

    TAttributes::TAttributes(const TResponse* resp) {
        ReadResponse(resp);
    }

    void TAttributes::ReadResponse(const TResponse* resp) {
        if (!resp)
            throw TFatalError(NSessionCodes::INVALID_PARAMS, "NULL Response pointer");

        // need to clean old values first
        ClearValues();

        // Then add all attrs coming from Response
        TResponse::TImpl* pImpl = resp->GetImpl();

        xmlConfig::Parts attrs(pImpl->GetParts("attributes/attribute"));
        TString type, val;

        for (int i = 0; i < attrs.Size(); ++i) {
            xmlConfig::Part item(attrs[i]);
            if (!item.GetIfExists("@type", type))
                throw TFatalError(NSessionCodes::UNKNOWN,
                                  "Bad blackbox response: missing attribute/@type");

            val = item.asString();

            Attributes_[type] = val;
        }
    }

    void TAttributes::Clear() {
        Attributes_.clear();
    }

    void TAttributes::ClearValues() {
        TContainerType::iterator p = Attributes_.begin();

        while (p != Attributes_.end())
            p++->second.clear();
    }

    TAttributes& TAttributes::operator<<(const TString& attr) {
        // adds the key if does not exist
        if (!attr.empty())
            Attributes_[attr];
        return *this;
    }

    const TString& TAttributes::Get(const TStringBuf attr) const {
        auto it = Attributes_.find(attr);

        if (it != Attributes_.end())
            return it->second;
        else
            return EMPTY_STR;
    }

    bool TAttributes::Has(const TStringBuf attr) const {
        return End() != Attributes_.find(attr);
    }

    TAttributes::TContainerType::const_iterator TAttributes::Begin() const {
        return Attributes_.begin();
    }

    TAttributes::TContainerType::const_iterator TAttributes::End() const {
        return Attributes_.end();
    }

    TAttributes::operator TOptions() const {
        TOptions r;
        return r << *this;
    }

    TOption TAttributes::Format() const {
        TString attributes_list;

        auto it = Attributes_.begin();

        if (it != Attributes_.end())
            attributes_list.append(it++->first);

        while (it != Attributes_.end()) {
            attributes_list.append(1, ',');
            attributes_list.append(it++->first);
        }

        return TOption("attributes", attributes_list);
    }

    TOptions& operator<<(TOptions& options, const TAttributes& attrs) {
        if (!attrs.Empty())
            options << attrs.Format();
        return options;
    }

    // ==== Static constants ==============================================

    const TString EMPTY_STR;

    const TOptions OPT_NONE;

    const TOption OPT_REGNAME("regname", "yes");
    const TOption OPT_AUTH_ID("authid", "yes");
    const TOption OPT_FULL_INFO("full_info", "yes");

    const TOption OPT_VERSION2("ver", "2");
    const TOption OPT_MULTISESSION("multisession", "yes");

    const TOption OPT_GET_SOCIAL_ALIASES("aliases", "getsocial");
    const TOption OPT_GET_ALL_ALIASES("aliases", "all");

    const TOption OPT_GET_ALL_EMAILS("emails", "getall");
    const TOption OPT_GET_YANDEX_EMAILS("emails", "getyandex");
    const TOption OPT_GET_DEFAULT_EMAIL("emails", "getdefault");

    const TOption OPT_GET_USER_TICKET("get_user_ticket", "yes");

}

// vi: expandtab:sw=4:ts=4
// kate: replace-tabs on; indent-width 4; tab-width 4;
