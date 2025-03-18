#include <kernel/qtree/richrequest/printrichnode.h>
#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>
#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/serialization/serializer.h>
#include <kernel/qtree/richrequest/nodeiterator.h>

#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/opt.h>

#include <util/generic/flags.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/memory/tempbuf.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/subst.h>
#include <util/system/defaults.h>


enum EUnpackingMode {
    UM_UNESCAPE      = 0x01,
    UM_PRINT_REQUEST = 0x02,
    UM_PRINT_MARKUP  = 0x04,
    UM_PRINT_RAW     = 0x08,
    UM_STREAMING     = 0x10,
    UM_UNSERIALIZE   = 0x20,
    UM_LF_ESCAPE     = 0x40,
};


static void Unpack(TString& qtree, IOutputStream& out, ECharset encoding, TFlags<EUnpackingMode> mode) {
    try {
        if (mode.HasFlags(UM_UNESCAPE)) {
            SubstGlobal(qtree, "+", "%2B");
            CGIUnescape(qtree);
        }

        if (mode.HasFlags(UM_PRINT_RAW)) {
            NRichTreeProtocol::TRichRequestNode proto;
            TRichTreeDeserializer().DeserializeToProto(qtree, true, proto);
            out << proto.Utf8DebugString() << Endl;
        }

        if (mode.HasFlags(UM_PRINT_REQUEST) || mode.HasFlags(UM_PRINT_MARKUP)) {
            TBinaryRichTree bin = DecodeRichTreeBase64(qtree);
            const EQtreeDeserializeMode dFlag = mode.HasFlags(UM_UNSERIALIZE) ? QTREE_UNSERIALIZE : QTREE_DEFAULT_DESERIALIZE;
            TRichTreePtr tree = DeserializeRichTree(bin, dFlag);

            if (mode.HasFlags(UM_PRINT_REQUEST)) {
                TString req = WideToChar(PrintRichRequest(tree.Get()), encoding);
                if (mode.HasFlags(UM_LF_ESCAPE))
                    SubstGlobal(req, "\n", "\\n");
                out << req << Endl;
            }

            if (mode.HasFlags(UM_PRINT_MARKUP)) {
                TString tmp;
                tree->SerializeToClearText(tmp);
                if (mode.HasFlags(UM_STREAMING)) {
                    size_t pos = 0;
                    while ((pos = tmp.find('\n', pos)) != TString::npos) {
                        size_t end = pos + 1;
                        while (end < tmp.size() && (tmp[end] == '\n' || tmp[end] == ' '))
                            end++;
                        tmp.erase(pos, end - pos);
                    }
                }
                if (encoding != CODES_UTF8) {
                    tmp = Recode(CODES_UTF8, encoding, tmp);
                }
                if (mode.HasFlags(UM_LF_ESCAPE))
                    SubstGlobal(tmp, "\n", "\\n");
                out << tmp << Endl;
            }
        }
    } catch (const yexception& e) {
        out << "error: " << e.what() << Endl;
    }
    if (&out == &Cout) {
        out.Flush();
    }
}


static void UnpackAll(IInputStream& in, IOutputStream& out, ECharset encoding, TFlags<EUnpackingMode> mode) {
    TString line;
    while (in.ReadLine(line)) {
        Unpack(line, out, encoding, mode);
    }
}


int main(int argc, char* argv[]) {
    try {
        ECharset encoding = CODES_UTF8;
        TFlags<EUnpackingMode> mode;
        mode |= UM_PRINT_MARKUP;
        mode |= UM_UNESCAPE;
        TString qtree;

        Opt opt(argc, argv, "e:q:rlspuw");
        int c;
        while ((c = opt.Get()) != EOF) {
            switch (c){
            case 'u':
                mode.RemoveFlags(UM_UNESCAPE);
                break;
            case 'e':
                encoding = CharsetByName(opt.Arg);
                break;
            case 'q':
                qtree = opt.Arg;
                break;
            case 'r':
                if (mode.HasFlags(UM_PRINT_REQUEST)) {
                    mode.RemoveFlags(UM_PRINT_MARKUP);
                }
                mode |= UM_PRINT_REQUEST;
                break;
            case 'l':
                mode |= UM_LF_ESCAPE;
                break;
            case 's':
                mode |= UM_STREAMING;
                break;
            case 'p':
                mode.RemoveFlags(UM_PRINT_MARKUP);
                mode.RemoveFlags(UM_PRINT_REQUEST);
                mode |= UM_PRINT_RAW;
                break;
            case 'w':
                mode |= UM_UNSERIALIZE;
                break;
            case '?':
            default:
                Cerr << "Usage:" << Endl;
                Cerr << "   unpackrichtree [-u] [-e encoding] [-r [-r]] [-s] [-p] [input output | -q tree]" << Endl;
                Cerr << "Options:" << Endl;
                Cerr << "   -u     treat input as raw base64 (default: url-encoded base64)" << Endl;
                Cerr << "   -r     print the request in human-readable form (twice: output nothing else)" << Endl;
                Cerr << "   -s     print JSON markup on a single line instead of pretty-printing" << Endl;
                Cerr << "   -p     print raw protobuf data" << Endl;
                Cerr << "   -l     replace LF in qtree with \\n escape sequence in output" << Endl;
                Cerr << "   -w     wizardify: try to restore in-wizard qtree (apply some heuristics from REQWIZARD-1173)" << Endl;
                return 2;
            }
        }

        if (opt.Ind == argc) {
            if (qtree) {
                Unpack(qtree, Cout, encoding, mode);
            } else {
                UnpackAll(Cin, Cout, encoding, mode);
            }
        } else if (opt.Ind == argc - 2 && !qtree) {
            TFileInput inf(argv[opt.Ind]);
            TFixedBufferFileOutput outf(argv[opt.Ind + 1]);
            UnpackAll(inf, outf, encoding, mode);
        } else {
            Cerr << "fatal: invalid number of arguments" << Endl;
            return 2;
        }
    } catch (const yexception& e) {
        Cerr << "fatal: " << e.what() << Endl;
        return 1;
    }
    return 0;
}
