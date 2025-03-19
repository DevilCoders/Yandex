#pragma once

#include <library/cpp/deprecated/calc_module/simple_module.h>

#include <util/folder/dirut.h>
#include <util/generic/hash.h>
#include <util/stream/file.h>

typedef THashMap<ui32, ui32> TC2PStore;

// Module for C2p incapsulation. Load C2P file.
class TC2pHolder : public TSimpleModule {
private:
    const TString C2pFileName;
    TC2PStore Store;

public:
    TC2pHolder(const TString& c2pFileName)
        : TSimpleModule("TC2pHolder")
        , C2pFileName(c2pFileName)
    {
        Bind(this).To<const TC2PStore*>(&Store, "c2p_output");
        Bind(this).To<&TC2pHolder::Init>("init");
        Y_VERIFY(NFs::Exists(C2pFileName), "[TC2pHolder] file <%s> not exists.", C2pFileName.data());
    }

private:
    void Init() {
        TIFStream geoc2pFile(C2pFileName);
        ui32 child, parent;

        TString str;
        while (geoc2pFile.ReadLine(str) && str.size()) {
            TStringBuf buf(str);
            child = FromString<ui32>(buf.NextTok('\t'));
            parent = FromString<ui32>(buf);
            Store[child] = parent;
        }
    }
};
