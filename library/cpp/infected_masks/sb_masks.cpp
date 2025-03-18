#include "sb_masks.h"

#include <util/stream/file.h>
#include <util/string/vector.h>
#include <util/string/strip.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/ysaveload.h>
#include <util/generic/hash.h>

#define SIMPLE_VALUES_LIMIT 6

static inline void WriteSmallLength(TVector<ui8>& data, ui32 length) {
    if (length < 0xFAU) {
        data.push_back(static_cast<ui8>(length));
    } else if (length < 0xFFFFU) {
        data.push_back(0xFAU);
        data.resize(data.size() + 2);
        *reinterpret_cast<ui16*>(&data[data.size() - 2]) = static_cast<ui16>(length);
    } else {
        data.push_back(0xFBU);
        data.resize(data.size() + 4);
        *reinterpret_cast<ui32*>(&data[data.size() - 4]) = static_cast<ui32>(length);
    }
}

static inline const ui8* ReadSmallLength(const ui8* position, ui32& result) {
    ui8 length = *position;
    ++position;
    if (length < 0xFAU) {
        result = length;
    } else if (length == 0xFAU) {
        result = *reinterpret_cast<const ui16*>(position);
        position += 2;
    } else {
        result = *reinterpret_cast<const ui32*>(position);
        position += 4;
    }
    return position;
}

static inline ui32 ReadSmallLength(const ui8*& position) {
    ui8 length = *position;
    ++position;
    if (length < 0xFAU) {
        return length;
    } else if (length == 0xFAU) {
        position += 2;
        return *reinterpret_cast<const ui16*>(position - 2);
    } else {
        position += 4;
        return *reinterpret_cast<const ui32*>(position - 4);
    }
}

template <class T>
static size_t GetDomainOffset(const T& host) {
    Y_ASSERT(host.find(':') == TString::npos);
    size_t pos = host.find_last_of('.');
    if (pos == TString::npos || pos == 0) {
        Y_ASSERT(pos != 0);
        return 0;
    }
    pos = host.find_last_of('.', pos - 1);
    if (pos == TString::npos)
        return 0;
    else
        return pos + 1;
}

static inline void RemoveSpecialCharacters(TString& s) {
    // Remove \n, \r and \t
    size_t pos = s.find_first_of("\n\r\t");
    while (pos != TString::npos) {
        s.erase(pos, 1);
        pos = s.find_first_of("\n\r\t", pos);
    }
}

// Repeatedly percent-unescape the URL then re-escape it once.
static void SafeEscapeUrl(TString& url) {
    TString unquoted = url;
    UrlUnescape(unquoted);
    while (unquoted != url) {
        url = unquoted;
        UrlUnescape(unquoted);
    }
    UrlEscape(url);
}

static TString CanonicalizeHostPrefix(const TStringBuf& prefix) {
    TString host(prefix);

    // Remove trailing dots, dots in the beginning have been removed earlier.
    while (!host.empty() && host.back() == '.') {
        host.pop_back();
    }

    // Replace consecutive dots with a single dot.
    for (size_t i = 1; i < host.length();) {
        if (host[i] == '.' && i + 1 < host.length() && host[i + 1] == '.') {
            host.erase(i, 1);
            continue;
        }
        ++i;
    }

    // Not implemented here:
    // If the hostname can be parsed as an IP address, it should be normalized to 4 dot-separated decimal values.
    // The client should handle any legal IP address encoding, including octal, hex, and fewer than 4 components.

    // Lowercase the whole string.
    host.to_lower();

    return host;
}

static TString CanonicalizeUrlPath(const TStringBuf& path) {
    TString canonicalPath(path);
    Strip(canonicalPath, canonicalPath);
    RemoveSpecialCharacters(canonicalPath);

    // Stripping off the fragment identifier
    size_t pos = canonicalPath.find('#');
    if (pos != TString::npos)
        canonicalPath = canonicalPath.substr(0, pos);

    SafeEscapeUrl(canonicalPath);

    if (!canonicalPath)
        return TString("/");

    // Replace /./ to /
    // Replace /path_component/../ to /
    // Replace consecutive slashes with a single slash
    if (Y_UNLIKELY(canonicalPath[0] != '/')) {
        canonicalPath.insert(canonicalPath.begin(), '/');
    }
    pos = 0;
    while (pos != TString::npos) {
        ++pos;
        if (pos == canonicalPath.size()) {
            break;
        } else if (Y_UNLIKELY(canonicalPath[pos] == '/')) {
            canonicalPath.erase(pos, 1);
            --pos;
        } else if (Y_UNLIKELY(canonicalPath[pos] == '.')) {
            ++pos;
            if (Y_UNLIKELY(pos == canonicalPath.size())) {
                break;
            } else if (canonicalPath[pos] == '/') {
                canonicalPath.erase(pos - 1, 2);
                pos -= 2;
            } else if (canonicalPath[pos] == '.' && (pos + 1 == canonicalPath.size() || canonicalPath[pos + 1] == '/')) {
                // Find start of the previous path component
                size_t endOfFragmentToDelete = pos + 1 < canonicalPath.size() ? pos + 1 : pos;
                if (pos > 2) {
                    size_t previous = canonicalPath.find_last_of('/', pos - 3);
                    Y_ASSERT(previous != TString::npos);
                    canonicalPath.erase(previous + 1, endOfFragmentToDelete - previous);
                    pos = previous;
                } else {
                    // Incorrect path, starting with /../, remove it without the preciding path component
                    canonicalPath.erase(1, pos + 1 < canonicalPath.size() ? 3 : 2);
                    pos = 0;
                }
            } else {
                pos = canonicalPath.find('/', pos);
            }
        } else {
            pos = canonicalPath.find('/', pos);
        }
    }

    return canonicalPath;
}

class TSafeBrowsingMasks::TInitializer: public TSafeBrowsingMasks::IInitializer {
public:
    struct TFirstPassValue {
        ui32 Next;
        TString Key;
        TString Value;

        TFirstPassValue(const TString& key, const TString& value)
            : Next(0)
            , Key(key)
            , Value(value)
        {
        }
    };

    struct TSecondPassValue {
        ui32 Next;
        TString Value;

        TSecondPassValue(const TString& value)
            : Next(0)
            , Value(value)
        {
        }
    };

public:
    TInitializer(TSafeBrowsingMasks& masks)
        : Masks(masks)
    {
        Masks.DomainHash.clear();
        Masks.Values.clear();
        Masks.MasksMap.clear();

        FirstPassValues.push_back(TFirstPassValue(TString(), TString())); // put phony value with index 0, because 0 is a special value
        SecondPassValues.push_back(TSecondPassValue(TString()));          // put phony value with index 0, because 0 is a special value
    }

    void WriteValues(TVector<unsigned char>& data, size_t offset1) {
        // Array of (length, value) ending with zero length value.
        size_t start = SecondPassValues[offset1].Next; // use Next to preserve initial values order
        size_t current = start;
        do {
            auto const& secondPassValue = SecondPassValues[current].Value;
            Y_ASSERT(secondPassValue.size() <= Max<ui32>());
            WriteSmallLength(data, secondPassValue.size());

            if (secondPassValue.size()) {
                size_t offset2 = data.size();
                data.resize(data.size() + secondPassValue.size());
                memcpy(&data[offset2], secondPassValue.data(), secondPassValue.size());
            }
            current = SecondPassValues[current].Next;
        } while (current != start);

        WriteSmallLength(data, 0);
    }

    // Add (mask, value) pairs by domain
    void AddMask(const TStringBuf& mask, const TStringBuf& value) override {
        TString canonicalMask(mask);
        SafeEscapeUrl(canonicalMask);

        TStringBuf host(canonicalMask);
        size_t pos = canonicalMask.find('/');
        if (pos != TString::npos) {
            host.Trunc(pos);
        }

        TString domain(host.substr(GetDomainOffset(host)));
        FirstPassValues.push_back(TFirstPassValue(pos == TString::npos ? (canonicalMask + '/') : canonicalMask, TString(value)));
        auto& domainHashValue = Masks.DomainHash[domain];
        AddValue(domainHashValue, FirstPassValues);
    }

    void Finalize() override;

private:
    void WriteDomainMasks(const TString& domain, const TMap<TString, ui32>& masksMap);

    // Join multiple values to a circular singly-linked list. The newly added value is in the end of the 'storage' container.
    // The 'index' is index of the last value of the current list in 'storage'. The order of values is preserved.
    template <class TStorage>
    void AddValue(ui32& index, TStorage& storage) {
        if (index == 0) {
            // First value
            storage.back().Next = storage.size() - 1;
        } else {
            // Repeated value for the same key.
            storage.back().Next = storage[index].Next;
            storage[index].Next = storage.size() - 1;
        }
        index = storage.size() - 1;
    }

private:
    TVector<TFirstPassValue> FirstPassValues;
    TVector<TSecondPassValue> SecondPassValues;
    TSafeBrowsingMasks& Masks;
};

void TSafeBrowsingMasks::TInitializer::WriteDomainMasks(const TString& domain, const TMap<TString, ui32>& masksMap) {
    auto& values = Masks.Values;
    if (masksMap.size() == 1 &&
        masksMap.begin()->first.StartsWith(domain.data(), domain.size()) &&
        masksMap.begin()->first.size() == domain.size() + 1 &&
        masksMap.begin()->first.back() == '/') {
        // Whole domain is banned.
        values.push_back(static_cast<ui8>(0));
        WriteValues(values, masksMap.begin()->second);
    } else {
        Y_ASSERT(masksMap.size() <= SIMPLE_VALUES_LIMIT);
        values.push_back(static_cast<ui8>(masksMap.size()));

        TVector<size_t> temporaryValuesOffsets;
        temporaryValuesOffsets.reserve(masksMap.size());
        for (const auto& it : masksMap) {
            // Divide mask to path and prefix
            const TString& mask = it.first;
            size_t pathStart = mask.find('/');
            size_t prefixEnd;

            if (pathStart == TString::npos) {
                // only prefix
                pathStart = mask.size();
                prefixEnd = GetDomainOffset(TStringBuf(mask));
            } else {
                prefixEnd = GetDomainOffset(TStringBuf(mask, 0, pathStart));
                ++pathStart;
            }
            if (prefixEnd > 0) {
                --prefixEnd;
            }

            TString canonicalPath = CanonicalizeUrlPath(TStringBuf(mask, pathStart, mask.size()));

            size_t length = 0;
            length += prefixEnd                  // + prefix
                      + canonicalPath.size() - 1 // + path without leading "/"
                      + 2 + sizeof(ui32);        // + 2 null characters + offset
            WriteSmallLength(values, length);

            // Write prefix
            while (prefixEnd > 0) {
                --prefixEnd;
                values.push_back(mask[prefixEnd]);
            }
            values.push_back('\0');

            //Write path
            // canonicalPath always starts with "/", we need to skip it.
            values.insert(values.end(), std::next(canonicalPath.cbegin()), canonicalPath.cend());
            values.push_back('\0');

            // The offset of the actual value will be written later.
            temporaryValuesOffsets.push_back(values.size());
            values.resize(values.size() + sizeof(ui32));
        }

        size_t i = 0;
        for (const auto& it : masksMap) {
            ui32 val = values.size() - temporaryValuesOffsets[i];
            memcpy(&values[temporaryValuesOffsets[i]], &val, sizeof(val));
            ++i;
            WriteValues(values, it.second);
        }
    }
}

void TSafeBrowsingMasks::TInitializer::Finalize() {
    Masks.Values.clear();
    Masks.Values.push_back(0); // phony value with index 0, to reserve 0 as a special offset

    TMap<TString, ui32> masksMap;
    for (auto& it1 : Masks.DomainHash) {
        masksMap.clear();
        size_t start = FirstPassValues[it1.second].Next;
        size_t current = start;
        do {
            SecondPassValues.push_back(TSecondPassValue(FirstPassValues[current].Value));
            auto& value = masksMap[FirstPassValues[current].Key];
            AddValue(value, SecondPassValues);

            current = FirstPassValues[current].Next;
        } while (current != start);

        static_assert(SIMPLE_VALUES_LIMIT < 0x80U, "");
        if (masksMap.size() > SIMPLE_VALUES_LIMIT) {
            // Complex case (too many subdomains or paths) it will be handled by the masks hash
            it1.second = 0;
            for (auto& it2 : masksMap) {
                Masks.MasksMap[it2.first] = Masks.Values.size();
                WriteValues(Masks.Values, it2.second);
            }
        } else {
            it1.second = Masks.Values.size();
            WriteDomainMasks(it1.first, masksMap);
        }
    }
}

void TSafeBrowsingMasks::Init(const TString& filePath) {
    TIFStream file(filePath);
    TString line;
    size_t lineNumber = 0;
    TSafeBrowsingMasks::TInitializer initializer(*this);
    while (file.ReadLine(line)) {
        ++lineNumber;
        size_t separator1 = line.find('\t');
        if (Y_UNLIKELY(separator1 == TString::npos)) {
            ythrow yexception() << "Can't parse line " << lineNumber << " in " << filePath;
        }
        size_t separator2 = line.find('\t', separator1 + 1);
        if (Y_UNLIKELY(separator2 == TString::npos)) {
            separator2 = line.size();
        }

        initializer.AddMask(TString(line.c_str(), separator1), TString(line.c_str() + separator1 + 1, separator2 - separator1 - 1));
    }
    initializer.Finalize();
    Y_VERIFY(Values.size() < Max<ui32>());
}

TSafeBrowsingMasks::IInitializer* TSafeBrowsingMasks::GetInitializer() {
    return new TSafeBrowsingMasks::TInitializer(*this);
}

bool TSafeBrowsingMasks::IsInfectedUrlImpl(const TStringBuf& hostname, const TStringBuf& path, bool recursiveCall, TMatchesType* matches) const {
    // Remove \r\n\t, unescape the %-escaped sequences, remove leading/trailing dots in the hostname
    bool needModification = false;
    for (auto a : hostname) {
        // Don't do anything with '#' for compartibility with the previous version of the library
        if (a <= char(32) || a >= char(127) || a == '%') {
            needModification = true;
            break;
        }
    }
    if (Y_UNLIKELY((needModification || (!hostname.empty() && (hostname[0] == '.' || hostname.back() == '.'))) && !recursiveCall)) {
        TString modifiedHostName(hostname);
        RemoveSpecialCharacters(modifiedHostName);

        SafeEscapeUrl(modifiedHostName);
        while (!modifiedHostName.empty() && modifiedHostName[0] == '.') {
            modifiedHostName.erase(0, 1);
        }
        while (!modifiedHostName.empty() && modifiedHostName.back() == '.') {
            modifiedHostName.pop_back();
        }

        return TSafeBrowsingMasks::IsInfectedUrlImpl(modifiedHostName, path, true, matches);
    }

    bool domainHasConsecutiveDots = false;
    size_t domainStart = 0;
    size_t pos = hostname.find_last_of('.');
    if (pos == TString::npos || pos == 0) {
        domainStart = 0;
    } else {
        size_t pos2 = hostname.find_last_of('.', pos - 1);
        if (pos2 == TString::npos) {
            domainStart = 0;
        } else {
            if (Y_UNLIKELY(pos2 == pos - 1)) {
                // double dot in the domain name
                domainHasConsecutiveDots = true;
                while (pos2 > 0 && hostname[pos2] == '.') {
                    --pos2;
                }
                if (pos2 > 0) {
                    pos2 = hostname.find_last_of('.', pos2);
                    if (pos2 == TString::npos) {
                        domainStart = 0;
                    } else {
                        domainStart = pos2 + 1;
                    }
                } else {
                    domainStart = 0;
                }
            } else {
                domainStart = pos2 + 1;
            }
        }
    }

    // Remove port
    size_t domainEnd = hostname.size();
    pos = hostname.find(':', domainStart);
    if (pos != TString::npos) {
        domainEnd = pos;
    }

    if (Y_UNLIKELY(domainHasConsecutiveDots)) {
        // Remove consecutive dots and do the search
        TString domain(hostname.substr(domainStart, domainEnd - domainStart));
        for (size_t i = domain.size() - 1; i > 0; --i) {
            if (domain[i] == '.' && domain[i - 1] == '.') {
                domain.erase(i, 1);
            }
        }
        return SearchMasks(domainStart > 0 ? TStringBuf(hostname, 0, domainStart - 1) : TStringBuf(),
                           domain, path, matches) != 0;
    } else {
        return SearchMasks(domainStart > 0 ? TStringBuf(hostname, 0, domainStart - 1) : TStringBuf(),
                           TStringBuf(hostname, domainStart, domainEnd - domainStart), path, matches) != 0;
    }
}

bool TSafeBrowsingMasks::IsInfectedUrl(const TStringBuf& hostname, const TStringBuf& urlpath, TMatchesType* matches) const {
    size_t hostStart = 0;

    // Remove scheme from the hostname
    size_t pos = hostname.find(':');
    if (pos != TString::npos && pos + 2 < hostname.size() && hostname[pos + 1] == '/' && hostname[pos + 2] == '/')
        hostStart = pos + 3;

    return IsInfectedUrlImpl(TStringBuf(hostname, hostStart, hostname.size() - hostStart), urlpath, false, matches);
}

// Warning: assuming that url is properly escaped
bool TSafeBrowsingMasks::IsInfectedUrl(const TStringBuf& url, TMatchesType* matches) const {
    // Split the requested url in hostnam and path.
    size_t hostStart = 0;
    size_t pathStart;

    size_t pos = url.find('/');
    if (pos == TString::npos) {
        pathStart = url.size();
    } else {
        if (pos > 0 && url[pos - 1] == ':') { // scheme found, remove it
            hostStart = pos + 1;
            if (hostStart < url.size() && url[hostStart] == '/')
                hostStart++;
            pathStart = url.find('/', hostStart);
            if (pathStart == TString::npos) {
                pathStart = url.size();
            }
        } else {
            pathStart = pos;
        }
    }

    return IsInfectedUrlImpl(TStringBuf(url, hostStart, pathStart - hostStart),
                             pathStart > 0 ? TStringBuf(url, pathStart, url.size() - pathStart) : TStringBuf(), false, matches);
}

bool TSafeBrowsingMasks::SearchMasksComplex(const TStringBuf& prefix, const TString& canonicalDomain, const TStringBuf& path, TMatchesType* matches) const {
    // Complex case (many subdomains or prefixes are banned).
    TString canonicalPrefix = CanonicalizeHostPrefix(prefix);
    TString canonicalPath = CanonicalizeUrlPath(TString(path));

    TVector<size_t> prefixOffsets;
    if (Y_LIKELY(!canonicalPrefix.empty())) {
        prefixOffsets.reserve(4);

        size_t pos = canonicalPrefix.rfind('.');
        while (prefixOffsets.size() < 3 && pos != TString::npos) {
            prefixOffsets.push_back(pos + 1);
            Y_ASSERT(pos > 0); // there can be no leading dots
            pos = canonicalPrefix.rfind('.', pos - 1);
        }
        if (prefixOffsets.size() < 3) {
            prefixOffsets.push_back(0);
        }
    }

    // Af first try to searh subdomains with minimal path, than add path components to the mask
    TString canonicalUrl;
    canonicalUrl.reserve(canonicalPrefix.size() + 1 + canonicalDomain.size() + canonicalPath.size());
    size_t domainStart = 0;
    if (!canonicalPrefix.empty()) {
        canonicalUrl.assign(canonicalPrefix.begin(), canonicalPrefix.end());
        canonicalUrl.push_back('.');
        domainStart = canonicalUrl.size();
    }
    canonicalUrl.append(canonicalDomain.begin(), canonicalDomain.end());
    size_t pathOffset = canonicalUrl.size();
    canonicalUrl.append(canonicalPath.begin(), canonicalPath.end());

    Y_ASSERT(!canonicalPath.empty());
    size_t pathEnd = 1;
    size_t pathComponent = 1;
    bool lastCheck = false;
    TString lookup;
    do {
        // Check domain + path
        lookup.assign(canonicalUrl, domainStart, pathOffset + pathEnd - domainStart);
        const auto& it1 = MasksMap.find(lookup);
        if (it1 != MasksMap.end()) {
            if (matches != nullptr) {
                matches->push_back(std::make_pair(lookup, it1->second));
            } else {
                return true;
            }
        }

        // Check up to 3 hostname prefixes + path
        for (auto offset : prefixOffsets) {
            lookup.assign(canonicalUrl, offset, pathOffset + pathEnd - offset);
            const auto& it2 = MasksMap.find(lookup);
            if (it2 != MasksMap.end()) {
                if (matches != nullptr) {
                    matches->push_back(std::make_pair(lookup, it2->second));
                } else {
                    return true;
                }
            }
        }
        // Check the full hostname + path
        if (prefixOffsets.size() == 3 && prefixOffsets[2] > 0) {
            lookup.assign(canonicalUrl, 0, pathOffset + pathEnd);
            const auto& it3 = MasksMap.find(lookup);
            if (it3 != MasksMap.end()) {
                if (matches != nullptr) {
                    matches->push_back(std::make_pair(lookup, it3->second));
                } else {
                    return true;
                }
            }
        }

        if (lastCheck) {
            break;
        }

        if (pathEnd < canonicalPath.size()) {
            pathEnd = canonicalPath.find('/', pathEnd);
            if (pathEnd == TString::npos) {
                pathEnd = canonicalPath.size();
            } else {
                ++pathEnd;
                ++pathComponent;
                if (pathComponent == 5) {
                    pathEnd = canonicalPath.size();
                }
            }
        } else {
            lastCheck = true;
            pathEnd = canonicalPath.find('?');
            if (pathEnd == TString::npos) {
                break;
            }
        }
    } while (true);

    return matches != nullptr && !matches->empty();
}

bool TSafeBrowsingMasks::SearchMasks(const TStringBuf& prefix, const TStringBuf& domain, const TStringBuf& path, TMatchesType* matches) const {
    TString canonicalDomain(domain);
    // Lowercase the domain, the remaining part of the canonicanization
    canonicalDomain.to_lower();

    // Search domain
    const auto& it = DomainHash.find(canonicalDomain);
    if (Y_LIKELY(it == DomainHash.end())) {
        return false;
    }

    if (matches != nullptr) {
        matches->clear();
    }

    if (Y_UNLIKELY(it->second == 0)) {
        return SearchMasksComplex(prefix, canonicalDomain, path, matches);
    }

    ui8 type = Values[it->second];
    if (type == 0) {
        // Whole domain is banned
        if (matches != nullptr) {
            matches->push_back(std::make_pair(TString(domain), it->second + 1));
            matches->back().first.push_back('/');
        }
        return true;
    } else {
        TString canonicalPrefix = CanonicalizeHostPrefix(prefix);
        TString canonicalPath = CanonicalizeUrlPath(TString(path));
        if (type < 0x80U) {
            const ui8* begin = &Values[0];
            const ui8* current = &Values[it->second + 1];
            Y_ASSERT(type <= SIMPLE_VALUES_LIMIT);
            for (size_t nMasks = type; nMasks > 0; --nMasks) {
                ui32 length;
                current = ReadSmallLength(current, length);
                const ui8* start = current;

                if (*current) {
                    // Compare prefix
                    int prefixIndex = canonicalPrefix.size() - 1;
                    while (prefixIndex >= 0 && *current != '\0' && canonicalPrefix[prefixIndex] == *current) {
                        ++current;
                        --prefixIndex;
                    }

                    if (*current != '\0' || (prefixIndex >= 0 && canonicalPrefix[prefixIndex] != '.')) {
                        // No match in the host name
                        current = start + length;
                        continue;
                    }
                }
                ++current;

                // Compare path
                size_t pathIndex = 1;
                while (pathIndex < canonicalPath.size() && *current != '\0' && canonicalPath[pathIndex] == *current) {
                    ++current;
                    ++pathIndex;
                }
                if (*current != '\0' ||
                    (pathIndex > 1 && pathIndex < canonicalPath.size() && (*(current - 1) != '/') && canonicalPath[pathIndex] != '?')) {
                    // No match in the path name
                    current = start + length;
                    continue;
                }
                ++current;

                // A match found
                if (matches == nullptr) {
                    return true;
                } else {
                    ui32 curValue;
                    memcpy(&curValue, current, sizeof(curValue));
                    ui32 offsetOfValues = curValue + (current - begin);
                    TString mask;
                    mask.reserve(canonicalPrefix.size() + 1 + canonicalDomain.size() + canonicalPath.size());
                    const char* prefix2 = reinterpret_cast<const char*>(start);
                    const char* path2 = prefix2 + strlen(prefix2) + 1;
                    for (const char* symbol = path2 - 2; symbol >= prefix2; --symbol) {
                        mask.push_back(*symbol);
                    }
                    if (!mask.empty()) {
                        mask.push_back('.');
                    }
                    mask.append(canonicalDomain);
                    mask.push_back('/');
                    mask.append(path2);
                    matches->push_back(std::make_pair(mask, offsetOfValues));
                    current = start + length;
                }
            }
            return matches != nullptr && !matches->empty();
        }
    }

    return false;
}

void TSafeBrowsingMasks::Save(IOutputStream* s) const {
    ::Save(s, ui32(Version));
    // Save hash
    ::Save(s, DomainHash.size());
    for (const auto& it : DomainHash) {
        ::Save(s, it.first);
        ::Save(s, it.second);
    }
    ::Save(s, Values.size());
    if (!Values.empty()) {
        ::SavePodArray(s, &Values[0], Values.size());
    }
    ::Save(s, MasksMap.size());
    for (const auto& it : MasksMap) {
        ::Save(s, it.first);
        ::Save(s, it.second);
    }
}

void TSafeBrowsingMasks::Load(IInputStream* s) {
    // Load the class version
    ui32 version;
    ::Load(s, version);
    if (version > Version) {
        ythrow yexception() << "In TSafeBrowsingMasks::Load. Received version is greater than the current one.";
    }

    // Load DomainHash
    size_t domainHashSize;
    ::Load(s, domainHashSize);
    DomainHash.clear();
    DomainHash.reserve(domainHashSize);
    while (domainHashSize > 0) {
        TString key;
        ::Load(s, key);
        ::Load(s, DomainHash[key]);
        --domainHashSize;
    }

    // Load Values
    size_t valuesSize;
    ::Load(s, valuesSize);
    Values.resize(valuesSize);
    if (valuesSize > 0) {
        ::LoadPodArray(s, &Values[0], Values.size());
    }

    // Load MasksMap
    size_t masksMapSize;
    ::Load(s, masksMapSize);
    MasksMap.clear();
    MasksMap.reserve(masksMapSize);
    while (masksMapSize > 0) {
        TString key;
        ::Load(s, key);
        ::Load(s, MasksMap[key]);
        --masksMapSize;
    }
}

int TSafeBrowsingMasks::operator&(IBinSaver& f) {
    ui32 version = Version;
    f.Add(1, &version);
    if (f.IsReading() && version > Version) {
        ythrow yexception() << "In TSafeBrowsingMasks::operator&. Received version is greater than the current one.";
    }

    f.Add(2, &DomainHash);
    f.Add(3, &MasksMap);
    f.Add(4, &Values);
    return 0;
}

void TSafeBrowsingMasks::ReadValues(size_t offset, TVector<TString>& result) const {
    const ui8* current = &Values[offset];
    for (ui32 length = ReadSmallLength(current); length > 0; length = ReadSmallLength(current)) {
        result.push_back(TString(reinterpret_cast<const char*>(current), length));
        current += length;
    }
}
