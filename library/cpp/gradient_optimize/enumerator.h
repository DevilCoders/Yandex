#pragma once

#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/maybe.h>
#include <util/string/cast.h>

class TEnumerator {
    THashMap<TString, int> Ids;
    TVector<TString> Names;
    int Id = 0;

public:
    TMaybe<int> Find(TStringBuf name) const {
        auto it = Ids.find(name);
        if (it != Ids.end()) {
            return it->second;
        } else {
            return {};
        }
    }

    int Enum(TStringBuf name) {
        auto it = Ids.find(name);
        if (it != Ids.end()) {
            return it->second;
        } else {
            Names.push_back(ToString(name));
            return Ids[name] = Id++;
        }
    }

    TString Name(int id) const {
        return Names[id];
    }

    const TVector<TString>& GetNames() const {
        return Names;
    }

    int Size() const {
        return Id;
    }
};
