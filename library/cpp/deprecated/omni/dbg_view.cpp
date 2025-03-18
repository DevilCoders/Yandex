#include "dbg_view.h"
#include <util/string/hex.h>
#include <util/string/split.h>

namespace NOmni {
    class TNonePrinter: public IDebugViewer {
    public:
        TNonePrinter(TString msg = "")
            : Msg(msg)
        {
        }

    protected:
        void GetDbgLines(TStringBuf data, TVector<TString>* lines) const override {
            if (Msg.size())
                lines->push_back(Msg);
            lines->push_back("[DATA] Size: " + ToString(data.size()));
        }

    private:
        TString Msg;
    };

    class TUtf8Printer: public IDebugViewer {
    protected:
        void GetDbgLines(TStringBuf data, TVector<TString>* lines) const override {
            StringSplitter(data).Split('\n').AddTo(lines);
        }
    };

    class THexPrinter: public IDebugViewer {
    protected:
        void GetDbgLines(TStringBuf data, TVector<TString>* lines) const override {
            lines->push_back(HexEncode(data.data(), data.size()));
        }
    };

    class TLittleEndianUIntPrinter: public IDebugViewer {
    protected:
        void GetDbgLines(TStringBuf data, TVector<TString>* lines) const override {
            switch (data.size()) {
                case 1:
                    lines->push_back(ToString(*reinterpret_cast<const ui8*>(data.data())));
                    break;
                case 2:
                    lines->push_back(ToString(*reinterpret_cast<const ui16*>(data.data())));
                    break;
                case 4:
                    lines->push_back(ToString(*reinterpret_cast<const ui32*>(data.data())));
                    break;
                case 8:
                    lines->push_back(ToString(*reinterpret_cast<const ui64*>(data.data())));
                    break;
                default:
                    ythrow yexception() << "bad uint size: " << data.size();
            }
        }
    };

    IDebugViewer* CreateDbgViewerByName(const TString& name) {
        if (name.empty())
            return new TNonePrinter();
        if (name == "utf8_printer")
            return new TUtf8Printer();
        if (name == "hex")
            return new THexPrinter();
        if (name == "uint_8_16_32_64_le_printer")
            return new TLittleEndianUIntPrinter();
        return new TNonePrinter("[Warning!] printer not found: " + name);
    }

}
