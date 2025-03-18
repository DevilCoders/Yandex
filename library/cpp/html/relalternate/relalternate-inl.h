#pragma once

#include <util/generic/buffer.h>
#include <util/generic/string.h>

namespace NRelAlternate {
    inline void AppendRelAlternate(TString& bufferToAppend, TString const& langHref) {
        if (langHref) {
            if (bufferToAppend) {
                bufferToAppend.append('\t');
            }
            bufferToAppend.append(langHref);
        }
    }

    inline void AppendRelAlternate(TBuffer& bufferToAppend, TString const& langHref) {
        if (langHref) {
            if (bufferToAppend.Size()) {
                bufferToAppend.Append('\t');
            }
            bufferToAppend.Append(langHref.data(), langHref.size());
        }
        if (bufferToAppend.Size()) {
            bufferToAppend.Append('\0');
        }
    }

}
