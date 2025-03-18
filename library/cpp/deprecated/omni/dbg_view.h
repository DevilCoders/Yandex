#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/stream/input.h>
#include <util/stream/file.h>
#include <util/system/filemap.h>
#include <util/generic/algorithm.h>
#include <util/string/split.h>
#include <util/memory/blob.h>

#include <library/cpp/json/json_reader.h>

#include "common.h"

namespace NOmni {
    class IDebugViewer {
    public:
        virtual void GetDbgLines(TStringBuf data, TVector<TString>* lines) const = 0;
        virtual ~IDebugViewer() {
        }

    protected:
    };

    /*
 * @param name                          Printer name.
 */
    IDebugViewer* CreateDbgViewerByName(const TString& name);

}
