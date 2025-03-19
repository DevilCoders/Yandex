#include "magic_input.h"

#include <util/generic/strbuf.h>
#include <util/stream/output.h>
#include <util/system/unaligned_mem.h>
#include <util/ysaveload.h>

const static TStringBuf MAGIC = "REMORPHC";
const static size_t HEADER_SIZE = MAGIC.size() + sizeof(ui8) + sizeof(ui16);

void TBinaryError::SetFilePath(const TString& filePath) {
    FilePath = filePath;
    *this << ", file: " << filePath;
}

TBinaryTypeError::TBinaryTypeError(ui8 expectedType, ui8 loadedType)
    : TBinaryError()
{
    *this << "Incompatible rules binary type, expected: " << expectedType << ", loaded: " << loadedType;
}

TBinaryTypeError::TBinaryTypeError(ui8 expectedType, ui8 loadedType, const TString& filePath)
    : TBinaryTypeError(expectedType, loadedType)
{
    SetFilePath(filePath);
}

TBinaryVersionError::TBinaryVersionError(ui16 expectedVersion, ui16 loadedVersion)
    : TBinaryError()
{
    *this << "Incompatible rules binary version, expected: " << expectedVersion << ", loaded: " << loadedVersion;
}

TBinaryVersionError::TBinaryVersionError(ui16 expectedVersion, ui16 loadedVersion, const TString& filePath)
    : TBinaryVersionError(expectedVersion, loadedVersion)
{
    SetFilePath(filePath);
}

TMagicCheck::TMagicCheck()
    : Buffer(HEADER_SIZE)
{
}

TMagicCheck::TMagicCheck(IInputStream& in)
    : Buffer(HEADER_SIZE)
{
    Init(in);
}

void TMagicCheck::Init(IInputStream& in) {
    Buffer.Proceed(in.Load(Buffer.Data(), HEADER_SIZE));
}

ui8 TMagicCheck::GetType() const {
    Y_ASSERT(HasMagic());
    return ReadUnaligned<ui8>(Buffer.Data() + MAGIC.size());
}

ui16 TMagicCheck::GetVersion() const {
    Y_ASSERT(HasMagic());
    return ReadUnaligned<ui16>(Buffer.Data() + MAGIC.size() + sizeof(ui8));
}

bool TMagicCheck::HasMagic() const {
    return Buffer.Size() == HEADER_SIZE
        && 0 == memcmp(MAGIC.data(), Buffer.Data(), MAGIC.size());
}

bool TMagicCheck::HasType(ui8 type) const {
    Y_ASSERT(HasMagic());
    return GetType() == type;
}

bool TMagicCheck::HasVersion(ui16 version) const {
    Y_ASSERT(HasMagic());
    return GetVersion() == version;
}

void TMagicCheck::CheckMagic() const {
    if (!HasMagic()) {
        throw TBinaryError() << "Loaded invalid binary data";
    }
}

void TMagicCheck::CheckType(ui8 type) const {
    if (!HasType(type)) {
        throw TBinaryTypeError(type, GetType());
    }
}

void TMagicCheck::CheckVersion(ui16 version) const {
    if (!HasVersion(version)) {
        throw TBinaryVersionError(version, GetVersion());
    }
}

void TMagicCheck::Check(ui8 type, ui16 version) const {
    CheckMagic();
    CheckType(type);
    CheckVersion(version);
}

TMagicInput::TMagicInput(IInputStream& stream)
    : TMagicCheck()
    , Slave(stream)
    , ReadSoFar(0)
{
    TMagicCheck::Init(Slave);
    MagicFound = HasMagic();
}

size_t TMagicInput::DoRead(void* buf, size_t len) {
    if (MagicFound || ReadSoFar >= Buffer.Size()) {
        return Slave.Read(buf, len);
    }
    size_t read = Min(Buffer.Size() - ReadSoFar, len);
    memcpy(buf, Buffer.Data() + ReadSoFar, read);
    ReadSoFar += read;
    return read;
}

void WriteMagic(IOutputStream& out, ui8 type, ui16 version) {
    out.Write(MAGIC.data(), MAGIC.size());
    ::Save(&out, type);
    ::Save(&out, version);
}
