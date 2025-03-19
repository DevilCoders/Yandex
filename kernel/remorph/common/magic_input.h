#pragma once

#include <util/generic/buffer.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/input.h>
#include <util/system/defaults.h>

class IOutputStream;

struct TBinaryError: public yexception {
    TString FilePath;

    void SetFilePath(const TString& filePath);
};

struct TBinaryTypeError: public TBinaryError {
    TBinaryTypeError(ui8 expectedType, ui8 loadedType);
    TBinaryTypeError(ui8 expectedType, ui8 loadedType, const TString& filePath);
};

struct TBinaryVersionError: public TBinaryError {
    TBinaryVersionError(ui16 expectedVersion, ui16 loadedVersion);
    TBinaryVersionError(ui16 expectedVersion, ui16 loadedVersion, const TString& filePath);
};

class TMagicCheck {
protected:
    TBuffer Buffer;

protected:
    void Init(IInputStream& in);

public:
    TMagicCheck();
    TMagicCheck(IInputStream& in);

    ui8 GetType() const;
    ui16 GetVersion() const;

    bool HasMagic() const;
    bool HasType(ui8 type) const;
    bool HasVersion(ui16 version) const;

    void CheckMagic() const;
    void CheckType(ui8 type) const;
    void CheckVersion(ui16 version) const;
    void Check(ui8 type, ui16 version) const;
};

class TMagicInput: public IInputStream, public TMagicCheck {
private:
    IInputStream& Slave;
    bool MagicFound;
    size_t ReadSoFar;
public:
    TMagicInput(IInputStream& stream);

    void Reset() {
        MagicFound = false;
        ReadSoFar = 0;
    }
protected:
    size_t DoRead(void* buf, size_t len) override;
};

void WriteMagic(IOutputStream& out, ui8 type, ui16 version);
