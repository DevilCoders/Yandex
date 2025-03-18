#include <library/cpp/html/face/event.h>

#include <tools/segutils/segcommon/dumphelper.h>
#include <tools/segutils/segcommon/data_utils.h>
#include <kernel/segutils/numerator_utils.h>

namespace NSegutils {

class TSDumpNumeratorHandler: public INumeratorHandler {
public:
    void InitSegmentator(const char*, const TOwnerCanonizer*, NSegm::NPrivate::TSegContext*) {}

    void OnMoveInput(const THtmlChunk& chunk, const TZoneEntry*, const TNumerStat& st) override {
        Cout << "{OnMoveInput}" << Endl;
        DumpEventInfo(chunk, st);
    }

    void OnTextEnd(const IParsedDocProperties*, const TNumerStat&) override {
        Cout << "{OnTextEnd}" << Endl;
    }

    void DumpEventInfo(const THtmlChunk& chunk, const TNumerStat& st) {
        DumpEventInfo(chunk);

        Cout << "\tTokenPosition=" << st.TokenPos.Break() << ':' << st.TokenPos.Word() << ':'
                << st.TokenPos.DocLength() << Endl;
    }

    void DumpEventInfo(const PARSED_FLAGS& fl) {
        Cout << "\tParsedType=" << GetConstName((PARSED_TYPE) fl.type);
        switch (fl.type) {
        default:
        case PARSED_ERROR:
        case PARSED_EOF:
            break;
        case PARSED_TEXT:
            Cout << "\tSpaceMode=" << GetConstName((SPACE_MODE) fl.space);
            break;
        case PARSED_MARKUP:
            Cout << "\tMarkupType=" << GetConstName((MARKUP_TYPE) fl.markup);
            break;
        }

        Cout << "\tTextWeight=" << GetConstName((TEXT_WEIGHT) fl.weight);
        Cout << "\tBreakType=" << GetConstName((BREAK_TYPE) fl.brk) << Endl;
    }

    void DumpEventInfo(const THtmlChunk& ch) {
        if (ch.text && ch.leng)
            Cout << TString(ch.text, ch.leng) << Endl;

        if (ch.Format)
            Cout << "\tFormat=" << (ui64) ch.Format;

        Cout << "\tTokenType=" << GetConstName(ch.GetLexType());

        if (ch.Tag) {
            Cout << "\tTagName=" << ch.Tag->lowerName;

            if (ch.Tag->flags & HT_empty)
                Cout << "\tHT_empty";
            if (ch.Tag->flags & HT_w0)
                Cout << "\tHT_w0";
            if (ch.Tag->flags & HT_subtab)
                Cout << "\tHT_subtab";
            if (ch.Tag->flags & HT_table)
                Cout << "\tHT_table";
        }

        Cout << Endl;

        DumpEventInfo(ch.flags);
    }

};

}

int main(int argc, char* argv[]) {
    using namespace NSegm;
    using namespace NSegutils;

    if (argc < 3) {
        Clog << "Usage: " << argv[0] << " configDir url [enchint]" << Endl;
        exit(1);
    }

    TParserContext ctx(argv[1]);

    THtmlDocument doc;
    doc.Url = argv[2];
    doc.HttpCharset = CharsetByName(argc >= 4 ? argv[3] : "");

    doc.Html = Cin.ReadAll();

    TSDumpNumeratorHandler num;
    ctx.SegNumerateDocument(&num, doc);
}
