#pragma once

#include <library/cpp/html/face/parsface.h>
#include <library/cpp/html/storage/queue.h>

class TRecognizerShell;
struct TDocInfoEx;

enum ERecognRes {
    RR_UNKNOWN,
    RR_PREDEFINED,
    RR_RECOGNIZED,
};

ERecognRes PrepareEncodingAndLanguage(NHtml::TSegmentedQueueIterator first, NHtml::TSegmentedQueueIterator last,
                                      IParsedDocProperties* docProps, TRecognizerShell* recogn, const TDocInfoEx* docInfo);
