#pragma once

#include <library/cpp/numerator/numerate.h>

#include "TextAndTitleSentences.h"


void NumerateHtml(const TString& html, INumeratorHandler& handler);

TSimpleSharedPtr<TTextAndTitleSentences> Html2Text(const TString& html);
