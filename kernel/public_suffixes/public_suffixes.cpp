#include "public_suffixes.h"

#include <kernel/url/url_canonizer.h>
#include <contrib/libs/libidn/lib/idna.h>
#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <util/memory/segmented_string_pool.h>
#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/split.h>

void TPublicSuffixStorage::AddState(const TStringBuf& name, ESuffixState state)
{
    THashMap<TStringBuf,ESuffixState>::iterator it = States.find(name);

    if (it != States.end()) {
        it->second = ESuffixState(it->second | state);
    } else {
        States[Names.append(name.begin(), name.size())] = state;
    }
}

TPublicSuffixStorage::ESuffixState TPublicSuffixStorage::GetState(const TStringBuf& name) const
{
    THashMap<TStringBuf,ESuffixState>::const_iterator it = States.find(name);

    if (it != States.end()) {
        return it->second;
    } else {
        return SS_UNKNOWN;
    }
}

void TPublicSuffixStorage::Clear()
{
    States.clear();
    Names.clear();
}

void TPublicSuffixStorage::FillFromStream(IInputStream& input)
{
    Clear();

    TPunycodeHostCanonizer punycodeHostCanonizer;

    TString line;
    TVector<TString> tmp;

    while (input.ReadLine(line)) {
        StringSplitter(line.data()).SplitByString("//").Limit(2).Collect(&tmp);

        if (tmp.empty()) {
            continue;
        }

        TString& key = tmp[0];

        if (key.size() == 0) {
            continue;
        }

        // punycode if needed
        if (!IsStringASCII(key.begin(), key.end())) {
            key = punycodeHostCanonizer.CanonizeHost(key);
        }

        if (key.StartsWith('!')) {
            AddState(key.data() + 1, SS_EXCEPT);
        } else if (key.StartsWith("*.")) {
            AddState(key.data() + 2, SS_EXTEND);
        } else {
            AddState(key, SS_OK);
        }
    }
}

void TPublicSuffixStorage::FillFromFile(const TString& fileName)
{
    TFileInput fileStream(fileName);
    FillFromStream(fileStream);
}

TStringBuf GetHostOwner(const TStringBuf& host, const TPublicSuffixStorage& storage)
{
    class TLocal {
    public:
        static size_t SafeFind(const TStringBuf& str, char c, size_t from = 0)
        {
            size_t pos = str.find(c, from);
            return (pos != TStringBuf::npos) ? pos : str.size();
        }

        static size_t SafeRFind(const TStringBuf& str, char c)
        {
            size_t pos = str.rfind(c);
            return (pos != TStringBuf::npos) ? pos : 0;
        }

        static size_t SafeRFind(const TStringBuf& str, char c, size_t from)
        {
            size_t pos = str.rfind(c, from);
            return (pos != TStringBuf::npos) ? pos : 0;
        }
    };

    if (host.size() == 0) {
        return TStringBuf();
    }

    size_t index = TLocal::SafeRFind(host, '.');

    TPublicSuffixStorage::ESuffixState state;

    bool lastWasOk = false;

    while (index > 0) {
        state = storage.GetState(host.Tail(index + 1));

        if (state == TPublicSuffixStorage::SS_UNKNOWN) {
            if (lastWasOk) {
                //++index;
                index = TLocal::SafeFind(host, '.', index + 1);
            }
            break;
        }

        if (state & TPublicSuffixStorage::SS_EXTEND) {
            index = TLocal::SafeRFind(host, '.', index);
            lastWasOk = false;
            continue;
        }

        if (state & TPublicSuffixStorage::SS_OK) {
            index = TLocal::SafeRFind(host, '.', index);
            lastWasOk = true;
            continue;
        }

        if (state & TPublicSuffixStorage::SS_EXCEPT) {
            index = TLocal::SafeFind(host, '.', index + 1);
            break;
        }

        // state == SS_OK
        break;
    }

    if (index > 0) {
        index = TLocal::SafeRFind(host, '.', index);
    }

    return (index > 0) ? host.Tail(index + 1) : host;
}
