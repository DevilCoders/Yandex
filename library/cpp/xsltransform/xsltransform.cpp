#include <contrib/libs/libxslt/libxslt/xslt.h>
#include <contrib/libs/libxslt/libxslt/transform.h>
#include <contrib/libs/libxslt/libxslt/xsltutils.h>
#include <contrib/libs/libxslt/libxslt/extensions.h>

#include <libxml/xinclude.h>

#include <contrib/libs/libexslt/libexslt/exslt.h>

#include <util/generic/buffer.h>
#include <util/generic/yexception.h>
#include <util/stream/str.h>
#include <util/stream/buffer.h>
#include <util/string/printf.h>

#include "xsltransform.h"

template <class T, void (*DestroyFun)(T*)>
struct TFunctionDestroy {
    static inline void Destroy(T* t) noexcept {
        if (t)
            DestroyFun(t);
    }
};

#define DEF_HOLDER(type, free) typedef THolder<type, TFunctionDestroy<type, free>> T##type##Holder
#define DEF_PTR(type, free) typedef TAutoPtr<type, TFunctionDestroy<type, free>> T##type##AutoPtr

// typedef THolder<xmlDoc, TFunctionDestroy<xmlDoc, xmlFreeDoc> > TxmlDocHolder;
DEF_HOLDER(xmlDoc, xmlFreeDoc);
DEF_HOLDER(xmlNode, xmlFreeNode);
DEF_HOLDER(xsltStylesheet, xsltFreeStylesheet);
DEF_PTR(xmlDoc, xmlFreeDoc);
DEF_PTR(xsltStylesheet, xsltFreeStylesheet);
// DEF_HOLDER(xmlOutputBuffer, xmlOutputBufferClose);
#undef DEF_HOLDER
#undef DEF_PTR

static class TXsltIniter {
public:
    TXsltIniter() {
        xmlInitMemory();
        exsltRegisterAll();
    }

    ~TXsltIniter() {
        xsltCleanupGlobals();
    }
} INITER;

struct TParamsArray {
    TVector<const char*> Array;

    TParamsArray(const TXslParams& p) {
        if (p.size()) {
            for (const auto& i : p) {
                Array.push_back(i.first.data());
                Array.push_back(i.second.data());
            }
            Array.push_back((const char*)nullptr);
        }
    }
};

class TXslTransform::TImpl {
private:
    TxsltStylesheetHolder Stylesheet;
    TString ErrorMessage;

private:
    static void XMLCDECL XmlErrorSilent(void*, const char*, ...) noexcept {
    }
    static void XMLCDECL XmlErrorSave(void* ctx, const char* msg, ...) noexcept {
        va_list args;
        va_start(args, msg);
        TString line;
        vsprintf(line, msg, args);
        TImpl* self = (TImpl*)ctx;
        self->ErrorMessage += line;
    }
    void InitError() {
        ErrorMessage.clear();
        xmlSetGenericErrorFunc(this, XmlErrorSave); // claimed to be thread-safe
    }

    void SaveResult(xmlDocPtr doc, xsltStylesheetPtr stylesheet, TBuffer& to) {
        xmlOutputBufferPtr buf = xmlAllocOutputBuffer(nullptr); // NULL means UTF8
        xsltSaveResultTo(buf, doc, stylesheet);
        if (buf->conv != nullptr) {
            to.Assign((const char*)xmlBufContent(buf->conv), xmlBufUse(buf->conv));
        } else {
            to.Assign((const char*)xmlBufContent(buf->buffer), xmlBufUse(buf->buffer));
        }
        xmlOutputBufferClose(buf);
    }

    void Transform(const TxmlDocHolder& srcDoc, TBuffer& dest, const TXslParams& params) {
        //xmlXIncludeProcessFlags(srcDoc.Get(), XSLT_PARSE_OPTIONS);
        if (!srcDoc)
            ythrow yexception() << "source xml is not valid: " << ErrorMessage;
        TxmlDocHolder res(xsltApplyStylesheet(Stylesheet.Get(), srcDoc.Get(), TParamsArray(params).Array.data())); // no params
        Y_ENSURE(!!res, "Failed to apply xslt!");
        SaveResult(res.Get(), Stylesheet.Get(), dest);
    }

public:
    TImpl(const TString& style, const TString& base = "") {
        InitError();
        TxmlDocHolder sheetDoc(xmlParseMemory(style.data(), style.size()));
        if (!!base) {
            xmlNodeSetBase(sheetDoc->children, (xmlChar*)base.c_str());
        }
        if (!sheetDoc)
            ythrow yexception() << "cannot parse xml of xslt: " << ErrorMessage;
        Stylesheet.Reset(xsltParseStylesheetDoc(sheetDoc.Get()));
        if (!Stylesheet)
            ythrow yexception() << "cannot parse xslt: " << ErrorMessage;
        // sheetDoc - ownership transferred to Stylesheet
        Y_UNUSED(sheetDoc.Release());
    }
    ~TImpl() {
        xmlSetGenericErrorFunc(nullptr, nullptr); // restore default (that is cerr echo)
    }

    void Transform(const TArrayRef<const char>& src, TBuffer& dest, const TXslParams& params) {
        InitError();
        TxmlDocHolder srcDoc(xmlParseMemory(src.data(), src.size()));
        Transform(srcDoc, dest, params);
    }

    xmlNodePtr ConcatTaskToNode(const TXmlConcatTask& task) {
        const xmlChar* nodeName = reinterpret_cast<const xmlChar*>(task.Element.empty() ? "node" : task.Element.data());
        TxmlNodeHolder taskNode(xmlNewNode(nullptr, nodeName));
        for (const TSimpleSharedPtr<IInputStream>& stream : task.Streams) {
            TBufferOutput out;
            TransferData(stream.Get(), &out);
            TxmlDocHolder document(xmlParseMemory(out.Buffer().Data(), out.Buffer().Size()));
            xmlNodePtr docRoot = xmlDocGetRootElement(document.Get());
            // nobody owns docRoot
            xmlUnlinkNode(docRoot);
            // taskNode owns docRoot
            xmlAddChild(taskNode.Get(), docRoot);
        }
        for (const TXmlConcatTask& childTask : task.Children) {
            xmlNodePtr child = ConcatTaskToNode(childTask);
            xmlAddChild(taskNode.Get(), child);
        }
        return taskNode.Release();
    }

    void Transform(const TXmlConcatTask& src, TBuffer& dest, const TXslParams& params) {
        InitError();
        TxmlDocHolder srcDoc(xmlNewDoc(nullptr));
        xmlNodePtr rootElem = ConcatTaskToNode(src);
        // now srcDoc owns rootElem, shouldn't free it
        xmlDocSetRootElement(srcDoc.Get(), rootElem);
        Transform(srcDoc, dest, params);
    }

    void SetFunction(const TString& name, const TString& uri, TxmlXPathFunction fn) {
        InitError();

        if (xsltRegisterExtModuleFunction(BAD_CAST name.data(), BAD_CAST uri.data(), (xmlXPathFunction)fn) < 0) {
            ythrow yexception() << "cannot register xsl function " << uri << ':' << name << ": " << ErrorMessage;
        }
    }
};

TXslTransform::TXslTransform(const TString& style, const TString& base)
    : Impl(new TImpl(style, base))
{
}

TXslTransform::TXslTransform(IInputStream& str, const TString& base)
    : Impl(new TImpl(str.ReadAll(), base))
{
}

TXslTransform::~TXslTransform() {
}

void TXslTransform::SetFunction(const TString& name, const TString& uri, TxmlXPathFunction fn) {
    Impl->SetFunction(name, uri, fn);
}

void TXslTransform::Transform(const TArrayRef<const char>& src, TBuffer& dest, const TXslParams& p) {
    Impl->Transform(src, dest, p);
}

void TXslTransform::Transform(const TXmlConcatTask& src, TBuffer& dest, const TXslParams& p) {
    Impl->Transform(src, dest, p);
}

void TXslTransform::Transform(IInputStream* src, IOutputStream* dest, const TXslParams& p) {
    TBufferOutput srcbuf;
    TransferData(src, &srcbuf);
    TBuffer result;
    Transform(TArrayRef<const char>(srcbuf.Buffer().data(), srcbuf.Buffer().size()), result, p);
    dest->Write(result.data(), result.size());
}

TString TXslTransform::operator()(const TString& src, const TXslParams& p) {
    TBuffer result;
    Transform(TArrayRef<const char>(src.data(), src.size()), result, p);
    return TString(result.data(), result.size());
}
