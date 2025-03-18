#pragma once

#include <tools/clustermaster/communism/client/client.h>

#include <util/generic/map.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>
#include <util/str_stl.h>
#include <util/string/cast.h>

#include <utility>

typedef std::pair<TString, size_t> TResourcesRequest;
typedef NCommunism::TClient<TResourcesRequest> TResourceManager;
typedef NCommunism::TDefinition TResourcesDefinition;

inline TResourceManager& GetResourceManager() {
    return *Singleton<TResourceManager>();
}

struct TResourcesIncorrect: yexception {};

struct TResourceValue {
    const TString Key;
    const TString Name;
    double Val;
    bool Auto;

    TResourceValue(const TString& key, const TString& name)
        : Key(key)
        , Name(name)
        , Val(0.0)
        , Auto(false)
    {
    }

    struct THash: private ::THash<TString> {
        size_t operator()(const TResourceValue& what) const noexcept {
            return ::THash<TString>::operator()(what.Key) ^ ::THash<TString>::operator()(what.Name);
        }
    };

    struct TEqualTo: private ::TEqualTo<TString> {
        bool operator ()(const TResourceValue& first, const TResourceValue& second) const noexcept {
            return ::TEqualTo<TString>::operator()(first.Key, second.Key) && ::TEqualTo<TString>::operator()(first.Name, second.Name);
        }
    };
};

#ifdef __clang__
// for static const double
#pragma clang diagnostic ignored "-Wgnu"
#endif // __clang__

class TResourcesStruct : THashTable<TResourceValue, TResourceValue, TResourceValue::THash, TIdentity, TResourceValue::TEqualTo, std::allocator<TResourceValue>> {
private:
    typedef THashTable<value_type, key_type, hasher, TIdentity, key_equal, std::allocator<TResourceValue>> TBase;

    static constexpr double TargetNameClaim = 1.0 / 1000.0;

    THolder<ISockAddr> Solver;
    TString TargetName;

    bool Auto;

public:
    typedef TBase::const_iterator const_iterator;
    typedef TBase::iterator iterator;

    using TBase::clear;
    using TBase::erase;
    using TBase::size;
    using TBase::begin;
    using TBase::end;

    TResourcesStruct()
        : TBase(7, hasher(), key_equal())
        , Auto(false)
    {
    }

    TResourcesStruct(const TResourcesStruct& right)
        : TBase(7, hasher(), key_equal())
        , Auto(false)
    {
        Y_VERIFY(right.empty() && !right.Solver.Get(), "calling TResourcesStruct copy ctor from non-empty instance");
    }

    bool GetAuto() const noexcept {
        return Auto;
    }

    void SetAuto(bool _auto) {
        Auto = _auto;
    }

    void Parse(const TString& what, const TString& defaultHost);

    TAutoPtr<TResourcesDefinition> CompoundDefinition() const;

    const ISockAddr& GetSolver() const noexcept {
        Y_VERIFY(Solver.Get(), "null solver pointer");
        return *Solver.Get();
    }

    bool IsLocalSolver() const {
        Y_VERIFY(Solver.Get(), "null solver pointer");
        return dynamic_cast<TSockAddrLocal*>(Solver.Get()) != nullptr;
    }

    void SetTargetName(const TString& targetName) {
        TargetName = targetName;
    }

    std::pair<iterator, bool> Insert(const TString& key) {
        return TBase::insert_unique(TResourceValue(key, TString()));
    }

    std::pair<iterator, bool> Insert(const TString& key, const TString& name) {
        return TBase::insert_unique(TResourceValue(key, name));
    }

    TResourceValue& FindOrInsert(const TString& key) {
        return TBase::find_or_insert(TResourceValue(key, TString()));
    }

    TResourceValue& FindOrInsert(const TString& key, const TString& name) {
        return TBase::find_or_insert(TResourceValue(key, name));
    }

    TResourceValue& operator[](const TString& key) {
        return FindOrInsert(key, TString());
    }

    void AddClaim(const TString& key, double val) {
        FindOrInsert(key).Val = val;
    }

    void AddSharedClaim(const TString& key, double val, const TString& name) {
        FindOrInsert(key, name).Val = val;
    }
};

enum EResourcesState {
    RS_DEFAULT,
    RS_BADVERSION,
    RS_CONNECTED,
    RS_CONNECTING,
    RS_INCOMPLETE,
    RS_INCORRECT,
    RS_REJECTED,
};

template <>
inline TString ToString<EResourcesState>(const EResourcesState& s) {
    switch (s) {
        case RS_DEFAULT: return "DEFAULT";
        case RS_BADVERSION: return "BADVERSION";
        case RS_CONNECTED: return "CONNECTED";
        case RS_CONNECTING: return "CONNECTING";
        case RS_INCOMPLETE: return "INCOMPLETE";
        case RS_INCORRECT: return "INCORRECT";
        case RS_REJECTED: return "REJECTED";
        default: ythrow yexception() << "unknown resources state";
    }
};
