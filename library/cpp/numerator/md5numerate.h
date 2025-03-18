#pragma once

#include "numerate.h"
#include <library/cpp/html/entity/htmlentity.h>

#include <library/cpp/digest/md5/md5.h>

class TMD5NumeratorHandler: public INumeratorHandler {
public:
    TMD5NumeratorHandler()
        : Attrs({"robots", "abstract", "yandex", "_ogtitle"})
    {
        Init();
    }

    void Init(bool strictMd5 = false) {
        WasBreak = true;
        StrictMd5 = strictMd5;

        Md5.Init();
    }

    void OnTokenStart(const TWideToken& tok, const TNumerStat& /*stat*/) override {
        if (WasBreak) {
            WasBreak = false;
            Md5.Update((unsigned char*)"\n", 1);
        }
        if (DetectNLPType(tok.SubTokens) == NLP_WORD) {
            for (size_t i = 0; i < tok.SubTokens.size(); i++) {
                Md5.Update((const unsigned char*)(tok.Token + tok.SubTokens[i].Pos), (unsigned int)tok.SubTokens[i].Len);
                Md5.Update((unsigned char*)"\0", 1);
            }
        } else {
            Md5.Update((const unsigned char*)tok.Token, (unsigned int)tok.Leng);
            Md5.Update((unsigned char*)"\0", 1);
        }
    }

    void OnTextEnd(const IParsedDocProperties* ps, const TNumerStat& /*stat*/) override {
        for (const char* attr : Attrs) {
            const char* prop = nullptr;
            ps->GetProperty(attr, &prop);
            if (prop) {
                UpdateSignature(prop);
            }
        }
    }

    void OnSpaces(TBreakType t, const wchar16* /*token*/, unsigned /*len*/, const TNumerStat&) override {
        if (IsSentBrk(t))
            WasBreak = true;
    }

    void OnMoveInput(const THtmlChunk&, const TZoneEntry* zone, const TNumerStat&) override {
        if (!zone)
            return;
        for (size_t i = 0; i < zone->Attrs.size(); ++i) {
            const TAttrEntry& attr = zone->Attrs[i];
            if (attr.Type == ATTR_URL || (StrictMd5 && attr.Type != ATTR_UNICODE_URL)) {
                UpdateSignature(attr.DecodedValue.data(), attr.DecodedValue.size());
            }
        }
    }

    TStringBuf GetSignature() {
        Md5.Final(Signature);
        return TStringBuf((char*)Signature, sizeof(Signature));
    }

private:
    void UpdateSignature(const wchar16* str, size_t len) {
        Md5.Update((const unsigned char*)str, (unsigned int)(len * sizeof(wchar16)));
    }

    void UpdateSignature(const char* str) {
        Md5.Update((const unsigned char*)str, (unsigned int)strlen(str));
    }

private:
    const TVector<const char* const> Attrs;
    unsigned char Signature[16];
    MD5 Md5;
    bool StrictMd5;
    bool WasBreak;
};
