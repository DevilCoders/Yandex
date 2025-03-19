#pragma once

#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <kernel/urlnorm/urlnorm.h>
#include <kernel/mirrors/mirrors.h>
#include <kernel/mirrors/mirrors_trie.h>

#include <library/cpp/deprecated/atomic/atomic.h>

struct hostset;
struct mirrors;
class segmented_string_pool;

class TMirrorResolver {
private:
    THolder<mirrors> Mirrors;

public:
    TMirrorResolver(const TString& mirrorsFilename);

    TString GetMainMirror(const TString& host) const;
};

class TMirrorMappedResolver {
private:
    THolder<mirrors_mapped> Mirrors;

public:
    TMirrorMappedResolver(const TString& mirrorsFilename);

    TString GetMainMirror(const TString& host) const;
};

class TOwnerResolver {
private:
    THolder<hostset> Owners;
    THolder<segmented_string_pool> OwnersPool;

public:
    TOwnerResolver(const TString& ownersFilename);
    ~TOwnerResolver();

    TString GetOwner(const TString& host) const;
};

class IHostCanonizer {
public:
    virtual TString CanonizeHost(const TString& host) const = 0;
    virtual bool IsResolvingMirrors() const;
    virtual ~IHostCanonizer() {}
};

typedef TSimpleSharedPtr<IHostCanonizer> THostCanonizerPtr;

/// Canonizing is performed in 3 stages:
/// 1) Before mirroring (IsResolvingMirrors() for the canonizer is false)
/// 2) Mirroring (IsResolvingMirrors() is true)
/// 3) After mirroring (IsResolvingMirrors() is false, the true value causes an exception)
class TCombinedHostCanonizer : public IHostCanonizer {
private:
    typedef TVector<THostCanonizerPtr> TCanonizers;
    TCanonizers BeforeMirroring;
    TCanonizers Mirroring;
    TCanonizers AfterMirroring;

    enum EFillingStage {
        FS_BEFORE_MIRRORING,
        FS_MIRRORING,
        FS_AFTER_MIRRORING
    };

    EFillingStage CurrentFillingStage;
    bool IsSmartAdding;
    bool IsEmpty;
public:
    TCombinedHostCanonizer();
    TCombinedHostCanonizer(const THostCanonizerPtr&);

    /// You can add a canonizer with this method.
    /// Initially it will add the canonizers to the first stage until IsResolvingMirrors() comes true.
    /// Then it will add the canonizers to the second stage until IsResolvingMirrors() comes false.
    /// Finally it adds the canonizers to the third stage. Exception is raised if IsResolvingMirrors() == true.
    void AddCanonizer(const THostCanonizerPtr& canonizer);

    /// Or you can add canonizers explicitly to a particular stage.
    /// DON'T MIX the two ways to add canonizers.
    /// Each of these three methods checks IsResolvingMirrors() before adding.
    void AddBeforeMirroringCanonizer(const THostCanonizerPtr& canonizer);
    void AddMirroringCanonizer(const THostCanonizerPtr& canonizer);
    void AddAfterMirroringCanonizer(const THostCanonizerPtr& canonizer);

    TString CanonizeBeforeMirrors(const TString& host) const;
    TString CanonizeMirrors(const TString& host) const;
    TString CanonizeAfterMirrors(const TString& host) const;


    bool IsResolvingMirrors() const override;
    TString CanonizeHost(const TString& host) const override;
};

typedef TSimpleSharedPtr<const TCombinedHostCanonizer> TCombinedHostCanonizerPtr;

class TMirrorHostCanonizer : public IHostCanonizer {
    TMirrorResolver MirrorResolver;

public:
    TMirrorHostCanonizer(const TString& mirrorsFilename);
    bool IsResolvingMirrors() const override;
    TString CanonizeHost(const TString& host) const override;
};

class TMirrorMappedHostCanonizer : public IHostCanonizer {
    TMirrorMappedResolver MirrorResolver;

public:
    TMirrorMappedHostCanonizer(const TString& mirrorsFilename);
    bool IsResolvingMirrors() const override;
    TString CanonizeHost(const TString& host) const override;
};

class TExtMirrorHostCanonizer : public IHostCanonizer {
    const TMirrorResolver& MirrorResolver;

public:
    TExtMirrorHostCanonizer(const TMirrorResolver& mirrorResolver);
    bool IsResolvingMirrors() const override;
    TString CanonizeHost(const TString& host) const override;
};

class TOwnerHostCanonizer : public IHostCanonizer {
private:
    const TString   OwnersFileName;
    TOwnerResolver OwnerResolver;

public:
    TOwnerHostCanonizer(const TString& ownersFilename);
    TString CanonizeHost(const TString& host) const override;

    TString GetOwnersFileName() const;
};

class TTrivialHostCanonizer : public IHostCanonizer {
public:
    TString CanonizeHost(const TString& host) const override;
};

/// Lies that it is resolving mirrors while doing nothing actually
class TFakeMirrorHostCanonizer : public TTrivialHostCanonizer {
public:
    bool IsResolvingMirrors() const override;
};

class TPunycodeHostCanonizer : public IHostCanonizer {
public:
    TString CanonizeHost(const TString& host) const override;
};

class TLowerCaseHostCanonizer : public IHostCanonizer {
public:
    TString CanonizeHost(const TString& host) const override;
};

class TStripWWWHostCanonizer : public IHostCanonizer {
    TString CanonizeHost(const TString& host) const override;
};

THostCanonizerPtr GetHostCanonizer(const TString& hostCanonizerParameters);

class TLastCachedHostCanonizer : public IHostCanonizer {
private:
    mutable TAtomic Lock;
    mutable TString PrevHost;
    mutable TString PrevResult;
    const IHostCanonizer* Canonizer;

    class TPoliteAtomicGuard {
    public:
        inline TPoliteAtomicGuard(TAtomic& atom) noexcept {
            Atom = AtomicTryLock(&atom) ? &atom : nullptr;
        }

        inline ~TPoliteAtomicGuard() {
            if (Atom != nullptr)
                AtomicUnlock(Atom);
        }

        inline bool WasAcquired() noexcept {
            return (Atom != nullptr);
        }

    private:
        TAtomic* Atom;
    };

public:
    TLastCachedHostCanonizer(const IHostCanonizer* canonizer);
    bool IsResolvingMirrors() const override;
    TString CanonizeHost(const TString& host) const override;
};

class TMirrorsMappedTrieCanonizer : public IHostCanonizer {
private:
    TMirrorsMappedTrie MirrorTrie;
public:
    TMirrorsMappedTrieCanonizer(const TString& filename, EPrechargeMode mode = PCHM_Disable);
    bool IsResolvingMirrors() const override;
    TString CanonizeHost(const TString& host) const override;
};

class TLocalMirrorsCanonizer : public IHostCanonizer {
private:
    THashMap<TString, TString> Mirrors;
public:
    TLocalMirrorsCanonizer(const TString& localMirrorsFileName);
    bool IsResolvingMirrors() const override;
    TString CanonizeHost(const TString& host) const override;
};

class TReverseDomainCanonizer : public IHostCanonizer {
public:
    TString CanonizeHost(const TString& url) const override;
};

bool IsGoodHost(const TString& host);

class TWrongUrlException : public yexception {
};

TString GetCanonicalUrl(const char* sourceUrl);
TString GetCanonicalUrl(const IHostCanonizer* canonizer, const TString& sourceUrl);
TString GetCanonicalUrlRight(const IHostCanonizer* canonizer, const TString& sourceUrl);
