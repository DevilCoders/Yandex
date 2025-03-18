#pragma once

#include "messages.h"

#include <util/folder/path.h>
#include <util/generic/ptr.h>

class TMasterConfigSource {
private:
    TFsPath Path;
    TString Content;
public:
    explicit TMasterConfigSource(const TFsPath& path)
        : Path(path)
    { }
    explicit TMasterConfigSource(const TString& content)
        : Content(content)
    { }

    bool IsPath() const {
        return Path.IsDefined();
    }

    const TFsPath& GetPath() const {
        Y_VERIFY(IsPath(), "must be path");
        return Path;
    }

    const TString& GetContent() const {
        Y_VERIFY(!IsPath(), "must be content");
        return Content;
    }
};


TAutoPtr<TConfigMessage> ParseMasterConfig(const TMasterConfigSource& configSource, const TString& masterHost, TIpPort masterHttpPort);

