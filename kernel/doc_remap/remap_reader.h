#pragma once

#include <util/system/defaults.h>
#include <util/system/file.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

class IRemapReader
{
public:
    virtual bool Remap(ui32 docId, ui32& remapped) = 0;

    inline ui32 Get(ui32 id) {
        ui32 result;
        if (Remap(id, result))
            return result;
        else {
            Y_ASSERT(0);
            return 0;
        }
    }

    inline ui32 SafeGet(ui32 id) {
        ui32 result;
        if (Remap(id, result)) {
            return result;
        }
        ythrow yexception() << "There is no remap for id " << id;
    }

    virtual ~IRemapReader() {};
};

class TTrivialRemapReader : public IRemapReader
{
    bool Remap(ui32 docId, ui32& result) override;
};

class TRemapReader : public IRemapReader
{
private:
    TFile FIn;
    ui32 Length;

public:
    TRemapReader(const char* filename);
    ~TRemapReader() override;
    bool Remap(ui32 docId, ui32& result) override;
    ui32 GetLength() const;
};

class TRemapReaderCached : public IRemapReader
{
private:
    typedef TVector<ui32> TRemapData;
    TRemapData RemapData;

public:
    TRemapReaderCached(const char* filename);
    bool Remap(ui32 docId, ui32& result) override;
    ui32 GetLength() const;
};

class TInvRemapReader : public IRemapReader
{
private:
    ui32 Length;
    typedef THashMap<ui32, ui32> TInvRemap;
    TInvRemap InvRemap;

public:
    TInvRemapReader(const char* fileName);
    ui32 GetLength() const;
    bool Remap(ui32 arcDocId, ui32& searchDocId) override;
};

ui32 GetRemap(const char* input, ui32 docId);
ui32 GetInvRemap(const char* input, ui32 docId);
