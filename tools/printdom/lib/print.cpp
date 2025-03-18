#include "print.h"

#include <library/cpp/html/blob/chunks.h>
#include <library/cpp/html/blob/document.h>
#include <library/cpp/html/print/print.h>
#include <library/cpp/json/json_writer.h>

#include <util/generic/map.h>
#include <util/generic/stack.h>
#include <util/stream/str.h>
#include <util/string/ascii.h>

using namespace NHtml;
using namespace NHtml::NBlob;

namespace {

static TString FrameId(const TElement& elem) {
    for (const auto& attr : elem.Attributes) {
        if (attr.Name == "data-frame-id") {
            return TString(" ") + attr.Name + "=" + attr.Value;
        }
    }
    return TString();
}

static bool IsSpace(const TStringBuf& str) {
    for (auto ci = str.begin(); ci != str.end(); ++ci) {
        if (!isspace(*ci)) {
            return false;
        }
    }

    return true;
}

static bool HasViewbound(const TElement& elem) {
    return elem.Viewbound.X || elem.Viewbound.Width ||
           elem.Viewbound.Y || elem.Viewbound.Height;
}


class TJsonPrinter : public INodeVisitor {
public:
    explicit TJsonPrinter(const TOptions& options)
        : Options_(options)
    {
        Scale_ = options.Scale;
    }

    const NJson::TJsonValue& Result() const {
        return Result_;
    }

    void OnDocumentStart() override
    { }

    void OnDocumentEnd() override
    { }

    void OnDocumentType(const TString& doctype) override {
        Result_["doctype"] = doctype;
    }

    void OnElementStart(const TElement& elem) override {
        NJson::TJsonValue value;

        value["tag"] = elem.Name;

        if (const TString& frameId = FrameId(elem)) {
            value["frameid"] = frameId;
        }
        if (Options_.Viewbound && HasViewbound(elem)) {
            const auto& vb = elem.Viewbound;

            value["viewbound"]["x"] = vb.X * Scale_;
            value["viewbound"]["y"] = vb.Y * Scale_;
            value["viewbound"]["width"] = vb.Width * Scale_;
            value["viewbound"]["height"] = vb.Height * Scale_;
        }
        if (Options_.Styles) {
            //EmitStyles(elem);
        }

        Elements_.push(value);
    }

    void OnElementEnd(const TString&) override {
        NJson::TJsonValue value(Elements_.top());

        Elements_.pop();

        if (Elements_.empty()) {
            Result_["children"].AppendValue(value);
        } else {
            Elements_.top()["children"].AppendValue(value);
        }
    }

    void OnText(const TString& text) override {
        if (Options_.EmptyText || !IsSpace(text)) {
            NJson::TJsonValue val;
            val["text"] = text;
            Elements_.top()["children"].AppendValue(val);
        }
    }

    void OnComment(const TString& text) override {
        NJson::TJsonValue val;
        val["comment"] = text;
        Elements_.top()["children"].AppendValue(val);
    }


private:
    TOptions Options_;
    NJson::TJsonValue Result_;
    TStack<NJson::TJsonValue> Elements_;
    float Scale_ = 1.0;
};

class TTreePrinter : public INodeVisitor {
public:
    TTreePrinter(const TOptions& options, IOutputStream& out)
        : Options_(options)
        , Output_(out)
    {
        Scale_ = options.Scale;
    }

    void OnDocumentStart() override
    { }

    void OnDocumentEnd() override
    { }

    void OnDocumentType(const TString& doctype) override {
        EmitSpaces();
        Output_ << "D: <" << doctype << ">\n";
    }

    void OnElementStart(const TElement& elem) override {
        EmitSpaces();
        Output_ << "E: <" << elem.Name << FrameId(elem) << ">  ";
        if (Options_.Viewbound && HasViewbound(elem)) {
            EmitViewbound(elem.Viewbound);
        }
        if (Options_.Styles) {
            EmitStyles(elem);
        }
        Output_ << "\n";

        Level_++;
    }

    void OnElementEnd(const TString&) override {
        Level_--;
    }

    void OnText(const TString& text) override {
        if (Options_.EmptyText || !IsSpace(text)) {
            EmitSpaces();
            Output_ << "T: " //(" << (float)fs * scale << ") "
                    << text
                    << "\n";
        }
    }

    void OnComment(const TString& text) override {
        EmitSpaces();
        Output_ << "C: <! " << text << " !>\n";
    }

private:
    void EmitSpaces() {
        for (size_t i = 0; i < Level_ * 2; ++i) {
            Output_ << " ";
        }
    }

    void EmitStyles(const TElement& elem) {
        TMap<TString, TString> map;

        for (const auto& pair : elem.ComputedStyle) {
            map[pair.Name] = pair.Value;
        }

        for (auto ci = map.begin(); ci != map.end(); ++ci) {
            Output_ << "  " << ci->first << " : " << ci->second;
        }
    }

    void EmitViewbound(const TRect& vb) {
        Output_ << "[" << vb.X * Scale_ << ", "
                       << vb.Y * Scale_ << "  "
                       << vb.Width * Scale_ << ":"
                       << vb.Height * Scale_
                << "]";
    }

#if 0
    static TFontSize FontSizes(const TNode& node, TFontSize base) {
        for (auto ci = node.computedstyle().begin(); ci != node.computedstyle().end(); ++ci) {
            if (AsciiEqualsIgnoreCase(ci->name(), "font-size")) {
                return TFontSize::Parse(ci->value(), base);
            }
        }

        return base;
    }
#endif

private:
    TOptions Options_;
    IOutputStream& Output_;
    size_t Level_ = 0;
    float Scale_ = 1.0;
};

}

int PrintAsJson(const TString& data, TOptions options) {
    auto doc = TDocument::FromString(
        data,
        TDocumentOptions().SetEnableStyles(options.Styles));

    options.Scale = doc->GetScaleFactor();

    TJsonPrinter printer(options);
    doc->EnumerateHtmlTree(&printer);

    Cout << NJson::WriteJson(printer.Result());
    return 0;
}

int PrintAsTree(const TString& data, TOptions options) {
    auto doc = TDocument::FromString(
        data,
        TDocumentOptions().SetEnableStyles(options.Styles));

    options.Scale = doc->GetScaleFactor();

    TTreePrinter printer(options, Cout);
    doc->EnumerateHtmlTree(&printer);

    return 0;
}

int PrintAsHtml(const TString& data) {
    THtmlPrinter p(Cout, TPrintConfig());
    if (!NBlob::NumerateHtmlChunks(TDocument::FromString(data), &p)) {
        Cerr << "fail to read packed document";
        return 1;
    }
    return 0;
}
