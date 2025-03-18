#pragma once

#include <tools/clustermaster/common/log.h>

#include <util/generic/hash.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/stream/output.h>

typedef THashSet<TString> TRecipients;

class TRecipientsWithTransport {
private:
    enum ETransport {
        MAIL = 0,
        SMS  = 1,
        JUGGLER  = 2,
        TELEGRAM = 3,
        JNS_CHANNEL = 4,
    };

private:
    typedef THashMap<ETransport, TAutoPtr<TRecipients>> TTransportRecipients;
    TTransportRecipients Recipients;

private:
    TRecipients& GetRecipients(ETransport transport) {
        auto it = Recipients.find(transport);
        if (it == Recipients.end()) {
            TAutoPtr<TRecipients> recipients(new TRecipients);
            it = Recipients.insert(TTransportRecipients::value_type(transport, recipients)).first;
        }
        return *(it->second.Get());
    }

    void AddRecipient(ETransport transport, const TString& account) {
        GetRecipients(transport).insert(account);
    }

    const TRecipients* GetRecipients(ETransport transport) const {
        const auto it = Recipients.find(transport);
        if (it == Recipients.end()) {
            return nullptr;
        }
        return it->second.Get();
    }

public:
    const TRecipients* GetMailRecipients() const {
        return GetRecipients(MAIL);
    }

    void AddMailRecipient(const TString& email) {
        AddRecipient(MAIL, email);
    }

    const TRecipients* GetSmsRecipients() const {
        return GetRecipients(SMS);
    }

    void AddSmsRecipient(const TString& account) {
        AddRecipient(SMS, account);
    }

    const TRecipients* GetTelegramRecipients() const {
        return GetRecipients(TELEGRAM);
    }

    void AddTelegramRecipient(const TString& account) {
        AddRecipient(TELEGRAM, account);
    }

    const TRecipients* GetJNSChannelRecipients() const {
        return GetRecipients(JNS_CHANNEL);
    }

    void AddJNSChannelRecipient(const TString& data) {
        AddRecipient(JNS_CHANNEL, data);
    }

    const TRecipients* GetJugglerEventTags() const {
        return GetRecipients(JUGGLER);
    }

    void AddJugglerEventTag(const TString& tag) {
        AddRecipient(JUGGLER, tag);
    }
};
