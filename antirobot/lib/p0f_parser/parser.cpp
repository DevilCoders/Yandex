#include "parser.h"

#include <antirobot/idl/cache_sync.pb.h>
#include <util/stream/format.h>
#include <util/stream/output.h>

#include <functional>

namespace P0fParser {
    TP0f StringToP0f(const TString& str) {
        return P0fParser::StringToP0f(TStringBuf{str});
    }

    TP0f StringToP0f(TStringBuf buf) {
        TP0f p0f;
        TStringBuf token;
        TStringBuf helper;

        std::function<void()> getVersion = [&] () {
            token = buf.NextTok(':');
            p0f.Version = FromString(token);
            Y_ENSURE(p0f.Version == 4 || p0f.Version == 6);
            token = buf.NextTok(':');
        };

        std::function<void()> getITTL = [&] () {
            p0f.ObservedTTL = FromString(token.NextTok('+'));
            if (ui8 value; token != "?") {
                value = FromString(token);
                p0f.ITTLDistance = value;
            }
            token = buf.NextTok(':');
        };

        std::function<void()> getOlen = [&] () {
            p0f.Olen = FromString(token);
            token = buf.NextTok(':');
        };

        std::function<void()> getMSS = [&] () {
            if (ui16 value; token != "?") {
                value = FromString(token);
                p0f.MSS = value;
            }
            token = buf.NextTok(':');
            helper = token.NextTok(',');
        };

        std::function<void()> getWSizeAndScale = [&] () {
            if (ui16 value; !helper.empty() && helper[0] != 'm') {
                value = FromString(helper);
                p0f.WSize = value;
            }
            if (ui16 value; token != "*") {
                value = FromString(token);
                p0f.Scale = value;
            }
            token = buf.NextTok(':');
        };

        std::function<void()> getOlayout = [&] () {
            if (!token.empty()) {
                const TVector<std::pair<bool*, TStringBuf>> params = {
                    {&p0f.LayoutNOP, "nop"},
                    {&p0f.LayoutMSS, "mss"},
                    {&p0f.LayoutWS, "ws"},
                    {&p0f.LayoutSOK, "sok"},
                    {&p0f.LayoutSACK, "sack"},
                    {&p0f.LayoutTS, "ts"}
                };
                bool any;
                do {
                    any = false;
                    helper = token.NextTok(',');
                    for(auto &&[field, param] : params) {
                        if (helper == param) {
                            *field = true;
                            any = true;
                            break;
                        }
                    }
                    if (!any) {
                        if (TStringBuf value; helper.AfterPrefix("eol+", value)) {
                            p0f.EOL = FromString(value);
                            any = true;
                        }
                    }
                    if (!any) {
                        if (TStringBuf value; helper.AfterPrefix("?", value)) {
                            p0f.UnknownOptionID = FromString(value);
                            any = true;
                        }
                    }
                    Y_ENSURE(any == true);
                } while(!token.empty());
            }
            token = buf.NextTok(':');
        };

        std::function<void()> getQuirks = [&] () {
            if (!token.empty()) {
                const TVector<std::pair<bool*, TStringBuf>> params = {
                    {&p0f.QuirksDF, "df"},
                    {&p0f.QuirksIDp, "id+"},
                    {&p0f.QuirksIDn, "id-"},
                    {&p0f.QuirksECN, "ecn"},
                    {&p0f.Quirks0p, "0+"},
                    {&p0f.QuirksFlow, "flow"},
                    {&p0f.QuirksSEQn, "seq-"},
                    {&p0f.QuirksACKp, "ack+"},
                    {&p0f.QuirksACKn, "ack-"},
                    {&p0f.QuirksUPTRp, "uptr+"},
                    {&p0f.QuirksURGFp, "urgf+"},
                    {&p0f.QuirksPUSHFp, "pushf+"},
                    {&p0f.QuirksTS1n, "ts1-"},
                    {&p0f.QuirksTS2p, "ts2+"},
                    {&p0f.QuirksOPTp, "opt+"},
                    {&p0f.QuirksEXWS, "exws"},
                    {&p0f.QuirksBad, "bad"}
                };
                bool any;
                do {
                    any = false;
                    helper = token.NextTok(',');
                    for(auto &&[field, param] : params) {
                        if (helper == param) {
                            *field = true;
                            any = true;
                            break;
                        }
                    }
                    Y_ENSURE(any == true);
                } while(!token.empty());
            }
        };

        std::function<void()> getPClass = [&] () {
            Y_ENSURE(buf.size() == 1);
            Y_ENSURE(buf == "0" || buf == "+");
            if (buf == "+") {
                p0f.PClass = true;
            }
        };

        getVersion();
        getITTL();
        getOlen();
        getMSS();
        getWSizeAndScale();
        getOlayout();
        getQuirks();
        getPClass();

        return p0f;
    }
} // namespace P0fParser

using namespace NAntiRobot;

template <>
void Out<P0fParser::TP0f>(IOutputStream& out, const P0fParser::TP0f& p0f) {
    out << int(p0f.Version) << ':' << int(p0f.ObservedTTL) << '+';
    if (p0f.ITTLDistance) {
        out << int(*p0f.ITTLDistance);
    } else {
        out << '?';
    }
    out << ':';
    out << int(p0f.Olen) << ':';
    if (p0f.MSS) {
        out << *p0f.MSS;
    } else {
        out << '*';
    }
    out << ':';
    if (p0f.WSize) {
        out << *p0f.WSize;
    } else {
        out << '*';
    }
    out << ',';
    if (p0f.Scale) {
        out << *p0f.Scale;
    } else {
        out << '*';
    }
    out << ':';
    {
        const TVector<std::pair<const bool*, TStringBuf>> params = {
            {&p0f.LayoutNOP, "nop"},
            {&p0f.LayoutMSS, "mss"},
            {&p0f.LayoutWS, "ws"},
            {&p0f.LayoutSOK, "sok"},
            {&p0f.LayoutSACK, "sack"},
            {&p0f.LayoutTS, "ts"}
        };
        int iteration = 0;
        for(auto &&[field, param] : params) {
            if (*field) {
                if (iteration > 0) {
                    out << ',';
                }
                out << param;
                ++iteration;
            }
        }
        if (p0f.EOL) {
            if (iteration > 0) {
                out << ',';
            }
            out << "eol+" << int(*p0f.EOL);
            ++iteration;
        }
        if (p0f.UnknownOptionID) {
            if (iteration > 0) {
                out << ',';
            }
            out << "?" << int(*p0f.UnknownOptionID);
        }
    }
    out << ':';
    {
        const TVector<std::pair<const bool*, TStringBuf>> params = {
            {&p0f.QuirksDF, "df"},
            {&p0f.QuirksIDp, "id+"},
            {&p0f.QuirksIDn, "id-"},
            {&p0f.QuirksECN, "ecn"},
            {&p0f.Quirks0p, "0+"},
            {&p0f.QuirksFlow, "flow"},
            {&p0f.QuirksSEQn, "seq-"},
            {&p0f.QuirksACKp, "ack+"},
            {&p0f.QuirksACKn, "ack-"},
            {&p0f.QuirksUPTRp, "uptr+"},
            {&p0f.QuirksURGFp, "urgf+"},
            {&p0f.QuirksPUSHFp, "pushf+"},
            {&p0f.QuirksTS1n, "ts1-"},
            {&p0f.QuirksTS2p, "ts2+"},
            {&p0f.QuirksOPTp, "opt+"},
            {&p0f.QuirksEXWS, "exws"},
            {&p0f.QuirksBad, "bad"}
        };
        int iteration = 0;
        for(auto &&[field, param] : params) {
            if (*field) {
                if (iteration > 0) {
                    out << ',';
                }
                out << param;
                ++iteration;
            }
        }
    }
    out << ':';
    if (p0f.PClass) {
        out << '+';
    } else {
        out << '0';
    }
}