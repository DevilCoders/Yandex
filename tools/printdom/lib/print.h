#pragma once

#include <util/generic/string.h>
#include <util/stream/output.h>

struct TOptions {
    float Scale = 1.0;
    bool EmptyText = false;
    bool Styles = false;
    bool Viewbound = true;
};

int PrintAsTree(const TString& data, TOptions options);

int PrintAsHtml(const TString& data);

int PrintAsJson(const TString& data, TOptions options);
