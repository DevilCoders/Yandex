#pragma once
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

class IInputStream;

class TGeoTree: public TAtomicRefCount<TGeoTree> {
public:
    class TMalformedException: public yexception {
    };
public:
    TGeoTree() = default;
    TGeoTree(IInputStream& in) {
        Load(in);
    }
    TGeoTree(const char* path) {
        Load(path);
    }

    virtual ~TGeoTree() {
    }

    void Load(IInputStream& in);
    void Load(const char* path);

    inline bool IsDescendant(size_t parent, size_t child) const {
        while (child && child != parent && child < Data.size())
            child = Data[child];
        return child == parent;
    }
protected:
    void AddEdge(size_t parent, size_t child);
private:
    TVector<size_t> Data;
};

typedef TIntrusivePtr<TGeoTree> TGeoTreePtr;

inline bool IsDescendant(const TGeoTreePtr& tree, size_t parent, size_t child) {
    if (!tree)
        return child == parent;
    return tree->IsDescendant(parent, child);
}
