#include "storage.h"

#include <tools/snipmake/steam/protos/steam.pb.h>
#include <tools/snipmake/steam/serp_parser/serp_xml_reader.h>
#include <tools/snipmake/steam/snippet_json_iterator/snippet_json_iterator.h>
#include <tools/snipmake/snippet_xml_parser/cpp_reader/snippet_xml_reader.h>

#include <contrib/libs/openssl/include/openssl/sha.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/stream/format.h>
#include <util/stream/mem.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>
#include <util/system/fstat.h>
#include <util/string/strip.h>


class TSHA256Hasher {
public:
    TSHA256Hasher();

    ~TSHA256Hasher()
    { }

    void Append(const void* buf, size_t len);

    TString GetHash();

private:
    SHA256_CTX Ctx;
};


TSHA256Hasher::TSHA256Hasher() {
    SHA256_Init(&Ctx);
}


void TSHA256Hasher::Append(const void* buf, size_t len) {
    SHA256_Update(&Ctx, buf, len);
}


TString TSHA256Hasher::GetHash() {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &Ctx);
    TStringStream res;
    for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        res << Hex(hash[i], HF_FULL);
    }
    return res.Str();
}


TString TFileStorage::ReqSnip2ProtoBuf(const NSnippets::TReqSnip& snipObj) {
    NSteam::NProto::TSnippet snipProtoBufObj;
    snipProtoBufObj.SetQuery(ReplaceBigUTF8Chars(snipObj.Query));
    snipProtoBufObj.SetB64QTree(snipObj.B64QTree);
    snipProtoBufObj.SetUrl(ReplaceBigUTF8Chars(WideToUTF8(snipObj.Url)));
    snipProtoBufObj.SetHilitedUrl(ReplaceBigUTF8Chars(WideToUTF8(snipObj.HilitedUrl)));
    snipProtoBufObj.SetRegion(snipObj.Region);
    snipProtoBufObj.SetTitleText(PreprocessMarkup(ReplaceBigUTF8Chars(WideToUTF8(
        snipObj.TitleText))));
    snipProtoBufObj.SetHeadline(PreprocessMarkup(ReplaceBigUTF8Chars(WideToUTF8(
        snipObj.Headline))));
    snipProtoBufObj.SetHeadlineSrc(snipObj.HeadlineSrc);
    for (size_t i = 0; i < snipObj.SnipText.size(); ++i) {
        NSteam::NProto::TFragment* fragment = snipProtoBufObj.AddFragments();
        fragment->SetText(PreprocessMarkup(ReplaceBigUTF8Chars(WideToUTF8(
            snipObj.SnipText[i].Text))));
    }
    snipProtoBufObj.SetLines(snipObj.Lines);
    snipProtoBufObj.SetExtraInfo(snipObj.ExtraInfo);
    TStringStream snipProtoBufStr;
    TZLibCompress compressor(&snipProtoBufStr, ZLib::Raw);
    snipProtoBufObj.SerializeToArcadiaStream(&compressor);
    compressor.Finish();
    return snipProtoBufStr.Str();
}


TString TFileStorage::PreprocessMarkup(const TString& text) {
    TString result = BrCutter.ReplaceAll(text);
    result = StrongToBReplacer.ReplaceAll(result);
    result = DoubleSpaceReplacer.ReplaceAll(result);
    return StripString(result);
}


TString TFileStorage::ReplaceBigUTF8Chars(const TString& text) {
    TStringStream cleaned_text;
    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(text.data());
    const unsigned char* end_ptr = reinterpret_cast<const unsigned char*>(text.data()) +
        text.size();
    while (ptr < end_ptr) {
        size_t char_length;
        TTempBuf char_buffer(4);
        RECODE_RESULT result = GetUTF8CharLen(char_length, ptr,
                                                       end_ptr);
        switch (result) {
            case RECODE_OK:
                if (char_length < 4) {
                    wchar32 unicode_char = 0;
                    result = ReadUTF8CharAndAdvance(unicode_char, ptr, end_ptr);
                    // GetUTF8CharLen is RECODE_OK => ReadUTF8Char is RECODE_OK
                    WriteUTF8Char(unicode_char, char_length,
                                           reinterpret_cast<unsigned char*>(char_buffer.Data()));
                    cleaned_text << TString(char_buffer.Data(), char_length);
                } else {
                    // MySQL utf8 does not support 4-byte chars
                    cleaned_text << Unsupported;
                    ptr += char_length;
                }
                break;
            case RECODE_EOINPUT:
                cleaned_text << Unsupported;
                ptr = end_ptr;
                break;
            default:
                cleaned_text << Unsupported;
                ++ptr;
                break;
        }
    }
    return cleaned_text.Str();
}


void TFileStorage::ReadSnippets(TSteamMMap& dest,
                                NSnippets::ISnippetsIterator& snipIt)
{
    while (snipIt.Next()) {
        const NSnippets::TReqSnip& snip = snipIt.Get();
        dest.insert(std::make_pair<TString, TString>(
            snip.Query + WideToUTF8(snip.Url),
            ReqSnip2ProtoBuf(snip)));
    }
}


TString TFileStorage::WriteSnippets(const TSteamMMap& sortedSnippets) {
    TSHA256Hasher hasher;
    for (TSteamMMap::const_iterator it = sortedSnippets.begin();
        it != sortedSnippets.end(); ++it)
    {
        hasher.Append((it->second).data(), (it->second).size());
    }
    TString hash = hasher.GetHash();
    TFsPath folder = GetFilepath(hash);
    folder.MkDirs();
    TFsPath poolFileName = folder / hash;
    TFsPath indexFileName = folder / (hash + ".idx");
    TFixedBufferFileOutput poolFile(poolFileName);
    TFixedBufferFileOutput indexFile(indexFileName);
    ui64 offset = 0;
    for (TSteamMMap::const_iterator it = sortedSnippets.begin();
        it != sortedSnippets.end(); ++it)
    {
        poolFile.Write(it->second);
        indexFile.Write(&offset, sizeof(ui64));
        offset += (it->second).size();
    }
    return hash;
}


TFileStorage::TFileStorage(const TString& storageRoot)
    : StorageRoot(storageRoot)
    , BrCutter("(?i)(?:<br(?:\\s*/?\\s*)?>\\s*)*(<br(?:\\s*/?\\s*)?>|…|$)", "\\1")
    , StrongToBReplacer("(?i)(</?)strong((?:\\s[^>]*)?>)", "\\1b\\2")
    , DoubleSpaceReplacer("(\\s)\\s+", "\\1")
    , Unsupported("\ufffd")
{ }


TFileStorage::~TFileStorage()
{ }

TString TFileStorage::StoreJSONSnipPool(const TString& jsonSnipFileName, bool isRCA) {
    try {
        TSteamMMap sortedSnippets;
        TFileInput snippetsInput(jsonSnipFileName);
        NSnippets::TSnippetsJSONIterator snipIt(&snippetsInput, isRCA);
        ReadSnippets(sortedSnippets, snipIt);
        return WriteSnippets(sortedSnippets);
    } catch (std::exception& e) {
        Cerr << e.what() << Endl;
    } catch (...) {
        Cerr << "TFileStorage::StoreJSONSnipPool: unknown error with file " <<
            jsonSnipFileName << Endl;
    }
    return "";
}


TString TFileStorage::StoreSnipPool(const TString& xmlSnipFileName) {
    try {
        TSteamMMap sortedSnippets;
        TFileInput snippetsInput(xmlSnipFileName);
        NSnippets::TSnippetsXmlIterator snipIt(&snippetsInput, false);
        ReadSnippets(sortedSnippets, snipIt);
        return WriteSnippets(sortedSnippets);
    } catch (std::exception& e) {
        Cerr << e.what() << Endl;
    } catch (...) {
        Cerr << "TFileStorage::StoreSnipPool: unknown error with file " <<
            xmlSnipFileName << Endl;
    }
    return "";
}


TString TFileStorage::StoreSerp(const TString& xmlSerpFileName) {
    try {
        TSteamMMap sortedSnippets;
        TFileInput snippetsInput(xmlSerpFileName);
        NSnippets::TSerpsXmlIterator snipIt(&snippetsInput);
        ReadSnippets(sortedSnippets, snipIt);
        return WriteSnippets(sortedSnippets);
    } catch (std::exception& e) {
        Cerr << e.what() << Endl;
    } catch (...) {
        Cerr << "TFileStorage::StoreSerp: unknown error with file " <<
            xmlSerpFileName << Endl;
    }
    return "";
}


TString TFileStorage::StoreRawFile(const TString& rawFileName) {
    try {
        TUnbufferedFileInput rawFile(rawFileName);
        size_t bufLen = 8192;
        TTempBuf buf(bufLen);
        TSHA256Hasher hasher;
        TStringStream compressedStr;
        TZLibCompress compressor(&compressedStr, ZLib::Raw);
        size_t read;
        while (read = rawFile.Read(buf.Data(), bufLen)) {
            hasher.Append(buf.Data(), read);
            compressor.Write(buf.Data(), read);
        }
        compressor.Finish();
        TString hash = hasher.GetHash();
        TFsPath folder = GetFilepath(hash);
        folder.MkDirs();
        TFixedBufferFileOutput storageFile(folder / hash);
        storageFile.Write(compressedStr.Str());
        return hash;
    } catch (std::exception& e) {
        Cerr << e.what() << Endl;
    } catch (...) {
        Cerr << "TFileStorage::StoreRawFile: unknown error with file " <<
            rawFileName << Endl;
    }
    return "";
}


TFsPath TFileStorage::GetFilepath(const TString& poolId) const {
    return StorageRoot / poolId.substr(0, 2);
}


TFsPath TFileStorage::GetFilename(const TString& poolId) const {
    return GetFilepath(poolId) / poolId;
}


TString TFileStorage::GetRawFile(const TString& fileId) {
    try {
        TUnbufferedFileInput storageFile(GetFilename(fileId));
        TZDecompress decompressor(&storageFile, ZLib::Raw);
        return decompressor.ReadAll();
    } catch (std::exception& e) {
        Cerr << e.what() << Endl;
    } catch (...) {
        Cerr << "TFileStorage::GetRawFile: unknown error with file " <<
            fileId << Endl;
    }
    return "";
}


void TFileStorage::DeleteFile(const TString& fileId) {
    try {
        GetFilename(fileId).DeleteIfExists();
        GetFilename(fileId + ".idx").DeleteIfExists();
    } catch (TIoException& e) {
        Cerr << e.what() << Endl;
    }
}


TSnippetIterator::TSnippetIterator(const TFileStorage& storage,
    const TString& poolId, size_t startSnipId, size_t endSnipId)
{
    try {
        InputSnipFile.Reset(new TUnbufferedFileInput(storage.GetFilename(poolId)));
        InputIndexFile.Reset(new TUnbufferedFileInput(storage.GetFilename(poolId).GetPath() + ".idx"));
        CurSnipId = startSnipId;
        EndSnipId = endSnipId;
        SnipCount = TFileStat((storage.GetFilename(poolId)
          .GetPath() + ".idx").data()).Size / sizeof(ui64);
        InputSnipFileSize = TFileStat((storage.GetFilename(poolId))
          .GetPath().data()).Size;

        if (EndSnipId > SnipCount) {
            EndSnipId = SnipCount;
        }
        if (CurSnipId < EndSnipId) {
            InputIndexFile->Skip(CurSnipId * sizeof(ui64));
            InputIndexFile->LoadOrFail(&Offset, sizeof(ui64));
            InputSnipFile->Skip(Offset);
            UpdateCurValue();
        } else {
            CurSnipId = EndSnipId;
        }
    } catch (std::exception& e) {
        Cerr << e.what() << Endl;
        SetInvalid();
    } catch (...) {
        Cerr << "TSnippetIterator::TSnippetIterator: unknown error with file " <<
            poolId << Endl;
        SetInvalid();
    }
}


TSnippetIterator::~TSnippetIterator()
{ }


bool TSnippetIterator::Valid() const {
   return CurSnipId != EndSnipId;
}


TString TSnippetIterator::Value() const {
    if (!Valid()) {
        Cerr << "TSnippetIterator::Value: failed to get next snippet! TSnippetIterator::Valid() == false!" << Endl;
        return "";
    }
    return CurValue;
}


void TSnippetIterator::UpdateCurValue() {
    ui64 snipLen = Offset;
    if (CurSnipId + 1 != SnipCount) {
        InputIndexFile->LoadOrFail(&Offset, sizeof(ui64));
    } else {
        Offset = InputSnipFileSize;
    }
    snipLen = Offset - snipLen;
    TTempBuf buf(snipLen);
    InputSnipFile->LoadOrFail(buf.Data(), snipLen);
    TMemoryInput bufInput(buf.Data(), snipLen);
    TZDecompress decompressor(&bufInput, ZLib::Raw);
    CurValue = decompressor.ReadAll();
}


void TSnippetIterator::Next() {
    try {
        if (!Valid()) {
            return;
        }
        ++CurSnipId;
        if (!Valid()) {
            CurValue = "";
            return;
        }
        UpdateCurValue();
    } catch (std::exception& e) {
        Cerr << e.what() << Endl;
        SetInvalid();
    } catch (...) {
        Cerr << "TSnippetIterator::Next: unknown error" << Endl;
        SetInvalid();
    }
}


size_t TSnippetIterator::GetSnipCount() const {
    return SnipCount;
}


void TSnippetIterator::SetInvalid() {
        CurSnipId = EndSnipId = 0;
        SnipCount = 0;
        CurValue = "";
}


TRE2Replacer::TRE2Replacer(const TString pattern, const TString replacement)
    : Pattern(pattern.data())
    , Replacement(replacement)
{ }


TString TRE2Replacer::ReplaceAll(const TString& text) const {
    std::string textCopy(text.data(), text.size());
    re2::RE2::GlobalReplace(&textCopy, Pattern, Replacement.data());
    return TString(textCopy.c_str(), textCopy.length());
}
