#pragma once

#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/memory/blob.h>
#include <util/stream/output.h>

namespace NSaasTrie {
    struct IDiskIO {
        virtual ~IDiskIO() = default;
        virtual TBlob Map(const TString& path) const = 0;
        virtual THolder<IOutputStream> CreateWriter(const TString& path) const = 0;
    };

    struct TDiskIO : IDiskIO {
        TBlob Map(const TString& path) const override;
        THolder<IOutputStream> CreateWriter(const TString& path) const override;
    };
}
