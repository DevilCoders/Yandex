#include "attrmap.h"

#include "mutdocattrs.h"

#include <util/folder/dirut.h>
#include <util/string/split.h>

NMemoryAttrMap::TMemoryConfigWrapper::TMemoryConfigWrapper()
    : NGroupingAttrs::TMetainfos(true, AttrsConfig, NGroupingAttrs::TConfig::AttrConfigReserv + 10)
    , AttrsConfig(NGroupingAttrs::TConfig::Search)
{ }

TMemoryAttrMap::TMemoryAttrMap(const TString& configDir, size_t size)
    : DocAttrs(size)
    , ConfigDir(configDir)
    , DocCount_(0)
{ }

namespace {

class TAttrSize {
private:
    size_t& Size;
public:
    TAttrSize(size_t& size)
        : Size(size)
    { }
    template<typename T>
    void Process() {
        Size = sizeof(T);
    }
};

class TAttrWriter {
private:
    char*& Data;
    TCateg Categ;
public:
    TAttrWriter(char*& data, TCateg categ)
        : Data(data)
        , Categ(categ)
    { }
    template<typename T>
    void Process() {
        *(T*)Data = static_cast<T>(Categ);
        Data += sizeof(T);
    }
};

}

NMemoryAttrMap::TOneDocAttrs::TDataPtr NMemoryAttrMap::TOneDocAttrs::Reset(const NGroupingAttrs::TConfig& config, const TVector<TMemoryCategPair>& docAttrs, bool update) {
    if (!Data)
        update = false;

    i32 lastLive = -1;
    size_t size = 0;
    if (!docAttrs.empty()) {
        const ui32 endAttr = Max<ui32>(GetAttrCount(), docAttrs.back().GetAttrNum() + 1);
        size_t itDocAttr = 0;
        ui32 docAttrNum = docAttrs[itDocAttr].GetAttrNum();
        size_t shift = 0;
        for (ui32 attr = 0; attr < endAttr; ++attr) {
            const size_t attrShift = attr - lastLive;
            if (itDocAttr < docAttrs.size() && attr == docAttrNum) {
                TCateg categ = docAttrs[itDocAttr].GetCateg();
                if (categ != END_CATEG) {
                    lastLive = attr;
                    size += sizeof(ui32) * attrShift;
                    SwitchType(config.AttrType(attr), TAttrSize(shift));
                } else
                    shift = 0;
                size_t curItDocAttr = itDocAttr;
                while (++itDocAttr < docAttrs.size() && docAttrs[itDocAttr].GetAttrNum() == docAttrNum) {
                };
                size += shift * (itDocAttr - curItDocAttr);
                if (itDocAttr < docAttrs.size())
                    docAttrNum = docAttrs[itDocAttr].GetAttrNum();
            } else if (update) {
                const char* begin = GetBegin(attr);
                const size_t dataSize = GetBegin(attr + 1) - begin;
                if (dataSize) {
                    size += sizeof(ui32) * attrShift + dataSize;
                    lastLive = attr;
                }
            }
        }
        size += sizeof(ui32);
    }

    if (lastLive == -1) {
        TDataPtr newData;
        if (!update)
            Data.Swap(newData);
        return newData;
    }

    TDataPtr newData(y_allocate(size));

    {
        ui32* offset = (ui32*)newData.Get();
        const char* startData = (const char*)newData.Get();
        char* data = (char*)newData.Get() + (lastLive + 2) * sizeof(ui32);
        size_t itDocAttr = 0;
        ui32 docAttrNum = docAttrs[itDocAttr].GetAttrNum();

        for (ui32 attr = 0; attr <= (ui32)lastLive; ++attr) {
            *offset++ = (ui32)(data - startData);
            if (itDocAttr < docAttrs.size() && attr == docAttrNum) {
                bool erase = docAttrs[itDocAttr].GetCateg() == END_CATEG;
                NGroupingAttrs::TConfig::Type type = erase ? NGroupingAttrs::TConfig::BAD : config.AttrType(attr);
                do {
                    if (!erase)
                        SwitchType(type, TAttrWriter(data, docAttrs[itDocAttr].GetCateg()));
                } while (++itDocAttr < docAttrs.size() && docAttrs[itDocAttr].GetAttrNum() == docAttrNum);
                if (itDocAttr < docAttrs.size())
                    docAttrNum = docAttrs[itDocAttr].GetAttrNum();
            } else if (update) {
                const char* begin = GetBegin(attr);
                const size_t dataSize = GetBegin(attr + 1) - begin;
                if (dataSize) {
                    memcpy(data, begin, dataSize);
                    data += dataSize;
                }
            }
        }
        *offset++ = (ui32)(data - startData);
    }
    Data.Swap(newData);
    return newData;
}
ui32 NMemoryAttrMap::TMemoryConfigWrapper::AddAttrIfAbsent(const char* attrName, NGroupingAttrs::TConfig::Type type) {
    return (ui32)AttrsConfig.AddAttr(attrName, type, true);
}

void TMemoryAttrMap::LoadC2P(const TString& attrList) {
    Y_VERIFY(ConfigDir.size(), "oops");
    NGroupingAttrs::TConfig tmpC2PConfig(NGroupingAttrs::TConfig::Search);
    tmpC2PConfig.InitFromStringWithTypes(attrList.data());
    TExistenceChecker ec(true);
    for (ui32 i = 0; i < tmpC2PConfig.AttrCount(); ++i) {
        const char* attrName = tmpC2PConfig.AttrName(i);
        NGroupingAttrs::TConfig::Type type = tmpC2PConfig.AttrType(i);
        if (*attrName) {
            ui32 attrNum = AddAttrIfAbsent(attrName, type);
            ProvideMetainfo(attrNum).Scan(ec.Check((ConfigDir + "/" + attrName + ".c2p").data()), NGroupingAttrs::TMetainfo::C2P);
        }
    }
}

ui32 TMemoryAttrMap::DocCount() const {
    return GetDocCount();
}

bool TMemoryAttrMap::DocCategs(ui32 docid, ui32 attrnum, TCategSeries& result, IRelevance*) const {
    return GetDocCategs(docid, attrnum, &result);
}

void TMemoryAttrMap::DocCategs(ui32 docid, const ui32 attrnums[], size_t attrcount, TCategSeries* results[], IRelevance*) const {
    for (size_t i = 0; i < attrcount; ++i) {
        GetDocCategs(docid, attrnums[i], results[i]);
    }
}

NGroupingAttrs::TVersion TMemoryAttrMap::Version() const {
    return 0;
}

NGroupingAttrs::TFormat TMemoryAttrMap::Format() const {
    return 0;
}

const NGroupingAttrs::TConfig& TMemoryAttrMap::Config() const {
    return AttrsConfig;
}

const NGroupingAttrs::TMetainfos& TMemoryAttrMap::Metainfos() const {
    return *this;
}

NGroupingAttrs::TConfig& TMemoryAttrMap::MutableConfig() {
    return AttrsConfig;
}

NGroupingAttrs::TMetainfos& TMemoryAttrMap::MutableMetainfos() {
    return *this;
}

TAutoPtr<NGroupingAttrs::IIterator> TMemoryAttrMap::CreateIterator(ui32 docid) const {
    return new TIterator(this, docid);
}

const void* TMemoryAttrMap::GetRawDocumentData(ui32) const {
    ythrow yexception() << "This grouping attribute format does not support retrieving raw document data";
}

bool TMemoryAttrMap::SetDocCateg(ui32, ui32, ui64) {
    ythrow yexception() << "writing doc attrs is not supported in this way for realtime search mode";
}
