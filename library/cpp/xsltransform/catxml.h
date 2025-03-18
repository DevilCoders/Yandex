#pragma once

#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

class IInputStream;

using TStreamVector = TVector<TSimpleSharedPtr<IInputStream>>;

struct TXmlConcatTask {
    TString Element;
    TStreamVector Streams;
    TVector<TXmlConcatTask> Children;

    TXmlConcatTask(const TString& element)
        : Element(element)
    {
    }
};
