#include "catfiltertrie.h"

#include <library/cpp/deprecated/mapped_file/mapped_file.h>
#include <library/cpp/uri/http_url.h>

#include <google/protobuf/message.h>
#include <google/protobuf/wire_format.h>

#include <util/ysaveload.h>
#include <util/generic/algorithm.h>
#include <util/system/filemap.h>
#include <util/system/mlock.h>

#include <algorithm>

TCatFilterTrie::TCatFilterTrie(const TString& filename, TMirrorsMappedTrie* mirrors, MapMode mode)
    : Mode(mode), MappedFile(new TMappedFile(filename))
    , Trie(new TTrie((char*)MappedFile->getData(), MappedFile->getSize()))
    , Mirrors(mirrors)
{
    if (Mode == mLocked)
        LockMemory(MappedFile->getData(), MappedFile->getSize());
}

TCatFilterTrie::TCatFilterTrie(char* data, size_t len, TMirrorsMappedTrie* mirrors)
    : Mode(mDefault)
    , Trie(new TTrie(data, len))
    , Mirrors(mirrors)
{
}

TCatFilterTrie::~TCatFilterTrie() {
    if (Mode == mLocked)
        UnlockMemory(MappedFile->getData(), MappedFile->getSize());
}

TCatFilterTrie::TTrie::TConstIterator TCatFilterTrie::Begin() const {
    return Trie->Begin();
}

TCatFilterTrie::TTrie::TConstIterator TCatFilterTrie::End() const {
    return Trie->End();
}

bool TCatFilterTrie::Next(TTrie::TConstIterator& it) const {
    if (it == Trie->End())
        return false;

    return ++it != Trie->End();

}

class TCatFiller {
private:
    ui8 Millions[MAXMILLIONS];
    THolder<TCatAttrs> Cats;
    size_t Level;

public:
    TCatFiller()
        : Cats(new TCatAttrs)
        , Level(1)
    {
        Zero(Millions);
    }

    void MoveUp() {
        Level++;
    }

    bool AllowCat(TCat cat) {
        if (cat >= 100000 && cat < 3000000)
            return true;

        unsigned int million = (unsigned int)(cat / 1000000);

        if (million == 4)
            return true;

        if (million > MAXMILLIONS)
            return false;

        if (Millions[million] > 0 && Millions[million] < Level)
            return false;

        if (million == 50) {
            unsigned int offset = (unsigned int)(cat % 1000000);
            if (offset <= MAXMILLIONS)
                Millions[offset] = Level;
            return false;
        }

        Millions[million] = Level;

        return true;
    }

    void AddCat(TCat cat) {
        if (AllowCat(cat))
            Cats->push_back(cat);
    }

    void AddCats(const TCatEntry_QueryCat& queryCat) {
        for (size_t i = 0; i < queryCat.CatSize(); i++)
            AddCat(queryCat.GetCat(i));

        if (Level > 1)
            return;

        for (size_t i = 0; i < queryCat.NotInheritedCatSize(); i++)
            AddCat(queryCat.GetNotInheritedCat(i));
    }

    TCatAttrsPtr Release() {
        Sort(Cats->begin(), Cats->end());
        Cats->erase(Unique(Cats->begin(), Cats->end()), Cats->end());
        return Cats.Release();
    }
};

TCatAttrsPtr TCatFilterTrie::Find(const TString& url) const noexcept {
    try {
        return DoFind(url);
    } catch (const yexception&) {
        return new TCatAttrs;
    }
}

bool IsQuery(const TString& str) {
    return str == "?";
}

void TCatFilterTrie::CollectQuery(const TTrie& trie, TVector<TString>::iterator begin, TVector<TString>::iterator end, TCatFiller& cats) const {
    for (TVector<TString>::iterator it = begin; it != end; it++) {
        TString key(" " + *it);
        TTrie sub = trie.FindTails(key.data(), key.size());
        if (sub.Begin() == sub.End()) {
            continue;
        }
        TTrie::TData entry;
        if (sub.Find("", 0, &entry)) {
            cats.AddCats(entry.querycat(0));
        }
        CollectQuery(sub, it + 1, end, cats);
    }
}

void TCatFilterTrie::CollectPath(TVector<TString>::iterator begin, TVector<TString>::iterator end, TCatFiller& cats) const {
    TString key = JoinStrings(begin, end, " ");

    TTrie::TPhraseMatchVector matches;
    Trie->FindPhrases(key, matches);

    if (matches.size() == 0)
        return;

    bool firstMatch = true;

    for (TTrie::TPhraseMatchVector::reverse_iterator it = matches.rbegin(); it != matches.rend(); it++) {
        TString match = key.substr(0, it->first);
        TString suffix = key.substr(it->first, key.size() - it->first);

        bool exactMatch = firstMatch && (match == key || suffix == " /");
        firstMatch = false;

        if (!exactMatch)
            cats.MoveUp();

        TCatEntry& entry = (it->second);
        cats.AddCats(entry.querycat(0));

        if (!match.EndsWith(" /"))
            cats.MoveUp();
    }
}

TCatAttrsPtr TCatFilterTrie::DoFind(const TString& url) const {
    TCatFiller cats;

    THolder<TVector<TString>> components(UrlToComponents(url, true, Mirrors));
    TVector<TString>::iterator queryBegin = FindIf(components->begin(), components->end(), IsQuery);

    if (queryBegin != components->end()) {
        TString key = JoinStrings(components->begin(), queryBegin + 1, " ");
        TTrie sub = Trie->FindTails(key.data(), key.size());
        if (sub.Begin() != sub.End())
            CollectQuery(sub, queryBegin + 1, components->end(), cats);
        cats.MoveUp();
    }

    CollectPath(components->begin(), queryBegin, cats);

    return cats.Release();
}

void TCatFilterTrie::Print() {
    for (TTrie::TConstIterator it = Trie->Begin(); it != Trie->End(); it++) {
        Cout << it.GetKey() << Endl;
        TCatEntry entry;
        it.GetValue(entry);
        PrintEntry(entry);
    }
}

void TCatFilterTrie::PrintEntry(const TCatEntry& entry) {
    for (ssize_t i = 0; i < entry.querycat_size(); i++) {
        const TCatEntry_QueryCat& qc = entry.querycat(i);
        Cout << "    query = " << qc.GetQuery() << Endl;
        if (qc.CatSize()) {
            Cout << "        cats = ";
            for (size_t j = 0; j < qc.CatSize(); j++)
                Cout << qc.GetCat(j) << " ";
            Cout << Endl;
        }
        if (qc.NotInheritedCatSize()) {
            Cout << "        not inherited cats = ";
            for (size_t j = 0; j < qc.NotInheritedCatSize(); j++)
                Cout << qc.GetNotInheritedCat(j) << " ";
            Cout << Endl;
        }
    }
}

THttpURL ParseUrl(const TString& str) {

    THttpURL url;

    if (
        url.Parse(
            str.find("://") != TString::npos ? str : ("http://" + str),
            THttpURL::FeatureSchemeFlexible
                | THttpURL::FeaturesNormalizeSet
                | THttpURL::FeatureAllowHostIDN
                | THttpURL::FeatureAuthSupported
        )
    ) {
        ythrow yexception() << "Failed to parse url: " << str;
    }
    if (url.IsNull(THttpURL::FlagHost)) {
        ythrow yexception() << "Not a valid url: " << str;
    }

    return url;
}

TVector<TString>* UrlToComponents(const TString& str, bool addRootDir, TMirrorsMappedTrie* mirrors) {

    THttpURL url(ParseUrl(str));

    TString scheme(url.GetField(THttpURL::FieldScheme));
    TString host(url.GetField(THttpURL::FieldHost));
    ui16 port(url.GetPort());

    if (mirrors) {

        TString mainHost;

        int flags = THttpURL::FlagHost;
        if (!scheme.empty() && scheme != "http") {
            // http-hosts contained in mirrors.trie without scheme
            flags |= THttpURL::FlagScheme;
        }
        mirrors->GetMain(url.PrintS(flags).c_str(), mainHost);

        THttpURL mainUrl(ParseUrl(mainHost));

        scheme = TString(mainUrl.GetField(THttpURL::FieldScheme));
        host = TString(mainUrl.GetField(THttpURL::FieldHost));
        port = mainUrl.GetPort();
    }

    THolder<TVector<TString>> components(new TVector<TString>);

    components->push_back(scheme);
    components->push_back(ToString<ui16>(port));

    TVector<TString> hostname = SplitString(host, ".");
    for (TVector<TString>::reverse_iterator it = hostname.rbegin(); it != hostname.rend(); it++)
        components->push_back(to_lower(*it));

    TString path(url.GetField(THttpURL::FieldPath));
    TString query(url.GetField(THttpURL::FieldQuery));
    TString fragment(url.GetField(THttpURL::FieldFragment));

    if (path == "/") {
        if (addRootDir || str.EndsWith('/') || !!query || !!fragment)
            components->push_back("/");
    } else {
        if (path.EndsWith('/'))
            path.erase(path.end() - 1);

        TVector<TString> path_ = SplitString(path, "/", 0, KEEP_EMPTY_TOKENS);
        for (TVector<TString>::iterator it = path_.begin(); it != path_.end(); it++) {
            components->push_back("/" + *it);
        }
    }

    if (!!query) {
        components->push_back("?");
        TVector<TString> params = SplitString(query, "&");
        Sort(params.begin(), params.end());
        for (TVector<TString>::iterator it = params.begin(); it != params.end(); it++) {
            components->push_back("&" + *it);
        }
    }

    if (!!fragment)
        components->push_back("#" + fragment);

    return components.Release();
}

void TCatPacker::UnpackLeaf(const char* p, TCatEntry& t) const {
    TMemoryInput in(p + sizeof(ui32), SkipLeaf(p) - sizeof(ui32));

    t.ParseFromArcadiaStream(&in);
}

void TCatPacker::PackLeaf(char* p, const TCatEntry& entry, size_t size) const {
    TMemoryOutput out(p, size + sizeof(ui32));

    Save<ui32>(&out, size);

    entry.SerializeToArcadiaStream(&out);
}

size_t TCatPacker::MeasureLeaf(const TCatEntry& entry) const {
    return entry.ByteSize() + sizeof(ui32);
}

size_t TCatPacker::SkipLeaf(const char* p) const {
    TMemoryInput in(p, sizeof(ui32));

    ui32 size;
    Load<ui32>(&in, size);

    return size;
}
