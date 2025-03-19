#pragma once

#include <util/system/defaults.h>
#include <util/generic/fwd.h>
#include <util/generic/ptr.h>
#include <util/charset/wide.h>

#include <library/cpp/langmask/langmask.h>

#include <kernel/tarc/iface/arcface.h>

class TBuffer;

namespace NIndexerCore {

class TArcTextBlockCreator {
public:
    TArcTextBlockCreator();
    ~TArcTextBlockCreator();

    // один раз в начале каждого документа
    void OpenDocument(TLangMask langs = TLangMask(LANG_RUS, LANG_ENG), ELanguage langprior = LANG_RUS);

    // Множество раз в середине
    void AddText(const wchar16* text, size_t len, const char* zoneName = nullptr);
    void AddText(const TString& text, const char* zoneName = nullptr);

    // Один раз в конце, записывает требуемый форматом архива буффер
    void CommitDocument(TBuffer* outBuf);

private:
    class TImpl;
    THolder<TImpl> Impl;
};

}
