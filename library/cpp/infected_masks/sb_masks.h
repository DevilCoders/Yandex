#pragma once

#include <library/cpp/binsaver/bin_saver.h>
#include <util/generic/hash_set.h>
#include <util/string/vector.h>

// New faster and more compact replacement for the TInfectedMasks library
// Known violations of the google safebrowsing API v3:
//     1. no ip address canonization. (legacy)
//     2. percent escapes in the hostname part are lowercased. (legacy)
//     3. unescaped '#' sign does not escaped back. (legacy)
//     4. don't do anything with username and password
//     5. whitespaces in the beginning and in the end of the url path are removed (legacy)
class TSafeBrowsingMasks {
private:
    static const ui32 Version = 0;
    THashMap<TString, ui32> DomainHash;
    THashMap<TString, ui32> MasksMap;
    TVector<ui8> Values;

public:
    // TMatchesType is a vector of pairs (the matched mask, the offset of values corresponding to this mask).
    // The offset can be passed to the ReadValues method to receive the actual values.
    using TMatchesType = TVector<std::pair<TString, ui32>>;

    //! @class IInitalizer
    //! @brief An interface to initialize TSafeBrowsingMasks instance.
    //!
    //! Provides an interface to implement initialization using arbitrary
    //! sources not covered by TSafeBrowsingMasks::Init.
    //!
    //! A typical initialization workflow looks like:
    //!  1. Get a pointer to IInitializer by calling TSafeBrowsingMasks::GetInitializer.
    //!  2. Call AddMask for every mask.
    //!  3. Finalize initialization by calling Finalize.
    //!  4. Destruct IInitialize instance.
    class IInitializer {
    public:
        virtual ~IInitializer(){};
        virtual void AddMask(const TStringBuf& mask, const TStringBuf& value) = 0;
        virtual void Finalize() = 0;
    };

    TSafeBrowsingMasks() {
    }
    TSafeBrowsingMasks(const TString& filePath) {
        Init(filePath);
    }

    void Init(const TString& filePath);
    IInitializer* GetInitializer();

    // IsInfectedUrl returns true if the specified url matches one of the masks.
    // When the optional argument 'matches' specified, all the matched masks are returnd.
    // In this case the search will be significantly slower.
    bool IsInfectedUrl(const TStringBuf& hostname, const TStringBuf& path, TMatchesType* matches = nullptr) const;
    bool IsInfectedUrl(const TStringBuf& url, TMatchesType* matches = nullptr) const;

    // Read values at the offset returned by GetOffsetInValues or GetOffsetInValuesComplex
    void ReadValues(size_t offset, TVector<TString>& result) const;

    void Save(IOutputStream* s) const;
    void Load(IInputStream* in);

    // For BinSaver
    int operator&(IBinSaver& f);

private:
    class TInitializer;

    bool IsInfectedUrlImpl(const TStringBuf& hostname, const TStringBuf& path, bool recursiveCall, TMatchesType* matches) const;

    bool SearchMasks(const TStringBuf& prefix, const TStringBuf& domain, const TStringBuf& path, TMatchesType* matches) const;
    bool SearchMasksComplex(const TStringBuf& prefix, const TString& canonicalDomain, const TStringBuf& path, TMatchesType* matches) const;
};
