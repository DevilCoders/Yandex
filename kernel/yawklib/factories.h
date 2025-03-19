#pragma once

#include "phrase_analyzer.h"
////////////////////////////////////////////////////////////////////////////////////////////////////
IRecognizerFactory *CreateDefaultRecognizerFactory();
IRecognizerFactory *CreateDefaultRecognizerFactory(const TString &filesPath);
////////////////////////////////////////////////////////////////////////////////////////////////////
// helpers for creating
IPhraseRecognizer* CreateStdFactoryToken(const TString &tk, const TUtf16String &wtk);
IPhraseRecognizer* CreateFileToken(const TString &tk);
IPhraseRecognizer* CreateGramToken(const TUtf16String &wtk);
