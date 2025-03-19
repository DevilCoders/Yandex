#include "config.h"

#include "attrname.h"

#include <util/string/cast.h>
#include <util/stream/file.h>
#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>
#include <util/string/split.h>

namespace NGroupingAttrs {

static constexpr TStringBuf indexAuxAttr = "$docid$";

TConfig::Type TConfig::GetType(TCateg value) {
    Y_ASSERT(sizeof(value) == 8);

    const TCateg one = 1;
    if (value >= -(one << 15) && value < (one << 15)) {
        return I16;
    }

    if (value >= -(one << 31) && value < (one << 31)) {
        return I32;
    }

    return I64;
}

TConfig::TConfig(Mode usage)
    : Usage(usage)
{
    Data.reserve(AttrConfigReserv); // to avoid array reallocation (need in realtime)
    if (Usage == Index) {
        AddAttr(indexAuxAttr.data(), I32);
    }
}

void TConfig::InitNonRealTime() {
    Sort(Data.begin(), Data.end(), std::greater());
    InitHash();
}

void TConfig::InitHash() {
    DataHash.clear();
    for (size_t i = 0; i < Data.size(); ++i) {
        DataHash[Data[i].Name] = ui32(i);
    }
}

void TConfig::InitUsage() {
    if (HasAttr(indexAuxAttr.data())) {
        Usage = Index;
    } else {
        Usage = Search;
    }
}

void TConfig::Init(const TVector<TString>& names, const TPackedType* types, size_t tlen, const bool* unique) {
    Y_ASSERT((Usage == Search && AttrCount() == 0) || (Usage == Index && AttrCount() == 1));

    Y_ENSURE(names.size() == tlen,
        "Invalid GroupingAttrs configuration, attribute names count (" <<
        names.size() << ") doesn't match type info length (" << tlen << "). ");

    size_t shift = (Usage == Index ? 1 : 0);
    Data.resize(names.size() + shift);
    for (size_t i = 0; i < names.size(); ++i) {
        Data[i + shift].Name = names[i];
        Data[i + shift].Size = static_cast<Type>(types[i]);
        if (unique)
            Data[i + shift].IsUnique = unique[i];
    }
    InitHash();
    InitUsage();
}

void TConfig::InitFromStringWithTypes(const char* str) {
    // ([:AttrName:](:[0-2])?)+
    Y_ASSERT((Usage == Search && AttrCount() == 0) || (Usage == Index && AttrCount() == 1));

    if (!str || !*str) {
        return;
    }

    TVector<TString> groups;
    StringSplitter(str).SplitBySet(" ,\t\r").SkipEmpty().Collect(&groups);

    if (groups.empty()) {
        ythrow yexception() << "Can't init config from string: no groups specified";
    }

    size_t shift = (Usage == Index ? 1 : 0);
    Data.resize(groups.size() + shift);
    TVector<TString> parts;
    for (size_t i = 0; i < groups.size(); ++i) {
        parts.clear();
        StringSplitter(groups[i]).Split(':').SkipEmpty().Collect(&parts);
        Data[i + shift].Name = parts[0];
        for (size_t j = 1; j < parts.size(); ++j) {
            if (strcmp(parts[j].data(), "named") == 0) {
                Data[i + shift].IsNamed = true;
            } else if (strcmp(parts[j].data(), "unique") == 0) {
                Data[i + shift].IsUnique = true;
            } else {
                Data[i + shift].Size = static_cast<Type>(FromString<ui32>(parts[j]));
            }
        }
    }

    InitNonRealTime();
    InitUsage();
}

void TConfig::InitFromStringWithTypes(const TString& str) {
    InitFromStringWithTypes(str.c_str());
}

TString TConfig::ToString() const {
    TStringStream ss;
    bool isFirst = true;
    for (ui32 attrnum = 0; attrnum < AttrCount(); ++attrnum) {
        if (strcmp(AttrName(attrnum), IndexAuxAttrName()) == 0)
            continue;

        if (isFirst)
            isFirst = false;
        else
            ss << " ";

        ss << AttrName(attrnum) << ":" << (int)AttrType(attrnum);
        if (IsAttrNamed(attrnum))
            ss << ":named";
        if (IsAttrUnique(attrnum))
            ss << ":unique";
    }
    return ss.Str();
}

static TConfig::Type Attrtype(const TString& str) {
    size_t semicolon = str.rfind(':');
    if (semicolon == TString::npos) {
        return TConfig::I32;
    }

    ui32 bytes = FromString<ui32>(str.data() + semicolon + 1, 1);
    if (bytes <= 2) {
        return TConfig::I16;
    } else if (bytes <= 4) {
        return TConfig::I32;
    } else {
        return TConfig::I64;
    }
}

void TConfig::InitFromStringWithBytes(const TString& str) {
    TStringInput input(str);
    InitFromStreamWithBytes(input);
}

void TConfig::InitFromStringWithBytes(const char* str) {
    InitFromStringWithBytes(TString(str));
}

void TConfig::InitFromFileWithBytes(const char* filename) {
    TFileInput input(filename);
    InitFromStreamWithBytes(input);
}
void TConfig::InitFromStreamWithBytes(IInputStream& input) {
    //([:AttrName:]\.d2c(:[1-8])\n)+
    TString line;

    while (input.ReadLine(line)) {
        DataHash[Attrname(line)] = Data.size();
        Data.push_back(TAttrData(Attrname(line), Attrtype(line), false, false));
    }

    InitNonRealTime();

    InitUsage();
}

void TConfig::MergeConfig(const TConfig& other) {
    for (ui32 attrnum = 0; attrnum < other.AttrCount(); ++attrnum) {
        const char* attrname = other.AttrName(attrnum);
        if (Usage == Search && strcmp(attrname, IndexAuxAttrName()) == 0) {
            continue;
        }
        if (HasAttr(attrname)) {
            ui32 num = AttrNum(attrname);
            if (Data[num].Size < other.Data[attrnum].Size) {
                Data[num].Size = other.Data[attrnum].Size;
            }
        } else {
            AddAttr(attrname, other.AttrType(attrnum), false, other.IsAttrUnique(attrnum));
        }
    }
}

const char* TConfig::IndexAuxAttrName() {
    return indexAuxAttr.data();
}

bool TConfig::IsAttrNamed(ui32 attrnum) const {
    Y_ASSERT(attrnum != NotFound && attrnum < AttrCount());
    return Data[attrnum].IsNamed;
}

bool TConfig::IsAttrUnique(size_t attrnum) const {
    Y_ASSERT(attrnum != NotFound && attrnum < AttrCount());
    return Data[attrnum].IsUnique;
}

int TConfig::AddAttr(const TString& attrname, Type type, bool isRealTime, bool isUnique) {
    const ui32 attrnum = AttrNum(attrname);
    if (attrnum != NotFound) {
        return (int)attrnum;
    }

    if (isRealTime) {
        Y_VERIFY(Data.size() + 1 <= AttrConfigReserv, "Possible relocation.");
    }

    DataHash[attrname] = Data.size();
    Data.push_back(TAttrData(attrname, type, false, isUnique));

    if (!isRealTime) {
        InitNonRealTime();
    }
    return (int)AttrNum(attrname);
}

bool TConfig::operator==(const TConfig& other) const {
    if (Data.size() != other.Data.size()) {
        return false;
    }
    for (size_t i = 0; i < Data.size(); ++i) {
        if (Data[i] != other.Data[i]) {
            return false;
        }
    }
    return true;
}

TConfig& TConfig::operator=(const TConfig& other) {
    Y_ENSURE(Usage == other.Usage, "Can't copy data from config with other usage mode " << ui32(other.Usage));
    Data = other.Data;
    DataHash = other.DataHash;
    return *this;
}

bool TConfig::HasAttr(TStringBuf attrname) const {
    return AttrNum(attrname) != NotFound;
}

ui32 TConfig::AttrNum(TStringBuf attrname) const {
    auto pAttr = DataHash.FindPtr(attrname);
    return pAttr ? *pAttr : NotFound;
}

const char* TConfig::AttrName(ui32 attrnum) const {
    Y_ASSERT(attrnum != NotFound && attrnum < AttrCount());
    return Data[attrnum].Name.data();
}

ui32 TConfig::AttrCount() const {
    return (ui32)Data.size();
}

TConfig::Type TConfig::AttrType(ui32 attrnum) const {
    Y_ASSERT(attrnum != NotFound && attrnum < AttrCount());
    return Data[attrnum].Size;
}

TCateg TConfig::AttrMaxValue(ui32 attrnum) const {
    Type type = AttrType(attrnum);
    switch (type) {
    case I64:
        return Max<i64>();
    case I32:
        return Max<i32>();
    case I16:
        return Max<i16>();
    default:
        ythrow yexception() << "bad type";
    }
}

bool TConfig::operator != (const TConfig& other) const {
    return !(operator == (other));
}

TConfig::TAttrData::TAttrData()
    : Size(I32)
    , IsNamed(false)
    , IsUnique(false)
{
}

TConfig::TAttrData::TAttrData(TString name, Type type, bool isNamed, bool isUnique)
    : Name(std::move(name))
    , Size(type)
    , IsNamed(isNamed)
    , IsUnique(isUnique)
{
}

bool TConfig::TAttrData::operator == (const TConfig::TAttrData& other) const {
    return (Name == other.Name && Size == other.Size && IsNamed == other.IsNamed && IsUnique == other.IsUnique);
}

bool TConfig::TAttrData::operator!=(const TConfig::TAttrData& other) const {
    return !(*this == other);
}

bool TConfig::TAttrData::operator>(const TConfig::TAttrData& other) const {
    if (Size != other.Size) {
        return Size > other.Size;
    }

    if (Name != other.Name) {
        return Name > other.Name;
    }

    if (IsNamed != other.IsNamed) {
        return IsNamed > other.IsNamed;
    }

    if (IsUnique != other.IsUnique) {
        return IsUnique > other.IsUnique;
    }

    return false;
}

}
