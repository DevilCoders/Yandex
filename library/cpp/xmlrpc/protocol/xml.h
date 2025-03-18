#pragma once

#include "value.h"

#include <library/cpp/xml/document/xml-document.h>

namespace NXmlRPC {
    static inline NXml::TConstNode FirstChild(NXml::TConstNode n) {
        n = n.FirstChild();

        while (!n.IsNull() && !n.IsElementNode()) {
            n = n.NextSibling();
        }

        return n;
    }

    static inline NXml::TConstNode NextSibling(NXml::TConstNode n) {
        do {
            n = n.NextSibling();
        } while (!n.IsNull() && !n.IsElementNode());

        return n;
    }

    static inline void Expect(const NXml::TConstNode& n, const TStringBuf& v) {
        if (n.Name() != v) {
            ythrow TXmlRPCError() << "can not parse xml - expect " << n.Name() << ", got " << v;
        }
    }
}
