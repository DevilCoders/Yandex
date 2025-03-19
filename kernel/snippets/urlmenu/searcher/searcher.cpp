#include <util/string/util.h>
#include <util/string/vector.h>
#include <util/charset/wide.h>

#include <kernel/snippets/urlmenu/common/urllib.h>

#include "searcher.h"

typedef TUrlMenuVector::value_type TUrlMenuItem;

namespace {
    bool MultipleTriesFind(const TVector<NUrlMenu::TUrlMenuTrie>& indexes, const char* key, size_t keylen, TString* value = nullptr) {
        for (const auto& index : indexes) {
            bool hasName = index.Find(key, keylen, value);
            if (hasName) {
                return true;
            }
        }
        return false;
    }
}

bool NUrlMenu::TSearcher::Search(const TVector<TUrlMenuTrie>& indexes, const TString& url, TUrlMenuVector& result) {
    TString normalizedUrl = NSitelinks::NormalizeUrl(url);
    result.clear();

    TVector<TString> hierarchy;
    NSitelinks::ListPageParents(normalizedUrl, hierarchy);
    hierarchy.insert(hierarchy.begin(), normalizedUrl);

    TString host = hierarchy.back();
    TString hostname = host; RemoveIfLast<TString>(hostname, '/');
    result.push_back(TUrlMenuItem(ASCIIToWide(host),
                                  UTF8ToWide(hostname)));
    hierarchy.pop_back();

    size_t printedPartLength = host.size() + 1;
    for (TVector<TString>::reverse_iterator i = hierarchy.rbegin(); i != hierarchy.rend(); ++i) {
        TString menuName;
        size_t urlLength = i->size();
        bool hasName = MultipleTriesFind(indexes, i->c_str(), urlLength, &menuName);
        if (!hasName && i->EndsWith('/')) {
            hasName = MultipleTriesFind(indexes, i->c_str(), urlLength - 1, &menuName);
            urlLength -= hasName ? 1 : 0;
        }

        if (hasName) {
            result.push_back(TUrlMenuItem(ASCIIToWide(TStringBuf(i->c_str(), urlLength)),
                                          UTF8ToWide(menuName)));
            printedPartLength = i->size();
        }
    }

    if (printedPartLength < normalizedUrl.size()) {
        TString slashedPage = normalizedUrl + "/";
        TString pageName;
        if (MultipleTriesFind(indexes, slashedPage.c_str(), slashedPage.size(), &pageName)) {
            result.push_back(TUrlMenuItem(ASCIIToWide(slashedPage),
                                          UTF8ToWide(pageName)));
            printedPartLength = slashedPage.size();
        }
    }

    if (result.size() <= 1)
        return false;

    if (printedPartLength < normalizedUrl.size()) {
        size_t tailLength = normalizedUrl.size() - printedPartLength;
        result.push_back(TUrlMenuItem(TUtf16String(),
            UTF8ToWide(normalizedUrl.c_str() + printedPartLength, tailLength)));
    } else {
        result.back().first.assign(ASCIIToWide(url));
    }

    return true;
}

bool NUrlMenu::TSearcher::Search(const TUrlMenuTrie& index, const TString& url, TUrlMenuVector& result) {
    const TVector<TUrlMenuTrie> indexes { index };
    return Search(indexes, url, result);
}

void NUrlMenu::TSearcher::Print(IOutputStream* out) {
    for (TUrlMenuTrie::TConstIterator i = Index.Begin(), mi = Index.End(); i != mi; ++i) {
        const TString url = (*i).first;
        const TString name = (*i).second;
        *out << url << "\t" << name << Endl;
    }
}

void NUrlMenu::CreateSearcherIndex(const TString& sourceFile, const TString& trieFile, bool minimized, bool verbose) {
        TSearcherIndexCreationOptions options;
        options.Minimize = minimized;
        options.Verbose = verbose;
        CreateSearcherIndex(sourceFile, trieFile, options);
}

void NUrlMenu::CreateSearcherIndex(const TString& sourceFile,
    const TString& trieFile,
    const NUrlMenu::TSearcherIndexCreationOptions& options)
{
    TFileInput in(sourceFile);
    TFixedBufferFileOutput out(trieFile);

    TCompactTrieBuilderFlags flags = CTBF_NONE;
    if (options.Verbose)
        flags |= CTBF_VERBOSE;
    if (options.Sorted)
        flags |= CTBF_PREFIX_GROUPED;
    THolder<TUrlMenuTrieBuilder> builder(new TUrlMenuTrieBuilder(flags));

    TString line;
    while (in.ReadLine(line)) {
        if (line.empty())
            continue;
        size_t sep = line.find('\t');
        TString key, value;
        if (sep != TString::npos) {
            if (options.Normalize)
                key = NSitelinks::NormalizeUrl(line.substr(0, sep));
            else
                key = line.substr(0, sep);

            value = line.substr(sep + 1);
            value = WideToUTF8(UTF8ToWide(value));
        }
        if (! key.empty())
            builder->Add(key.c_str(), key.size(), value);
    }

    if (options.Minimize) {
        TBufferOutput raw;
        size_t datalength = builder->Save(raw);
        if (options.Verbose)
            Cerr << "Data length (before compression): " << datalength << Endl;
        builder.Destroy();

        datalength = CompactTrieMinimize<TUrlMenuTrie::TPacker>(out, raw.Buffer().Data(), raw.Buffer().Size(), options.Verbose);
        if (options.Verbose)
            Cerr << "Data length (minimized): " << datalength << Endl;
    } else {
        size_t datalength = builder->Save(out);
        if (options.Verbose)
            Cerr << "Data length: " << datalength << Endl;
    }
}

