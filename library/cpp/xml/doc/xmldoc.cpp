#include "xmldoc.h"

#include <library/cpp/xml/init/init.h>

#include <util/stream/output.h>

namespace NXml {
    namespace {
        struct TInit {
            inline TInit() {
                InitEngine();
            }
        } initer;
    }

    class TXmlDoc::TImpl {
    private:
        TxmlXPathContextHolder XPathContext; // put it first to be deleted last
        TxmlDocHolder XmlDoc;

    public:
        TImpl() {
            XmlDoc.Reset(xmlNewDoc((xmlChar*)"1.0"));
            // XmlDoc->encoding = 0; // request no text conversion
        }
        TImpl(const TArrayRef<const char>& content) {
            XmlDoc.Reset(xmlParseMemory(content.data(), content.size()));
            if (!XmlDoc)
                ythrow TXmlError() << "cannot parse xml doc"; /// @todo: report error details
        }
        void Save(IOutputStream& to) {
            int saveOptions = XML_SAVE_FORMAT | XML_SAVE_NO_DECL;
            xmlBufferPtr buffer = xmlBufferCreate();
            xmlSaveCtxtPtr sctx = xmlSaveToBuffer(buffer, /* const char *encoding, */ nullptr, saveOptions);

            // sctx->handler = 0;
            // sctx->escape = 0;
            xmlSaveSetAttrEscape(sctx, nullptr);
            xmlSaveSetEscape(sctx, nullptr);

            xmlSaveDoc(sctx, XmlDoc.Get());
            xmlSaveClose(sctx);

            to.Write(buffer->content, buffer->use);

            xmlBufferFree(buffer);
        }

        TxmlXPathObjectPtr Select(const char* expr) {
            if (!XPathContext) {
                XPathContext.Reset(xmlXPathNewContext(XmlDoc.Get()));
                if (!XPathContext)
                    ythrow TXPathError() << "cannot create xpath context (which is strange)";
            }
            // ignore namespaces
            TxmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar*)expr, XPathContext.Get());
            if (!result)
                ythrow TXPathError() << "cannot apply xpath (incorrect expression?): " << expr;
            return result;
        }
        TxmlXPathObjectPtr Select(const char* expr, const xmlNodePtr context) {
            TxmlXPathContextHolder xpathContext(xmlXPathNewContext(XmlDoc.Get()));
            if (!xpathContext) {
                if (!xpathContext)
                    ythrow TXPathError() << "cannot create xpath context (which is strange)";
            }
            xpathContext->node = context;
            // ignore namespaces
            TxmlXPathObjectPtr result = xmlXPathEvalExpression((const xmlChar*)expr, xpathContext.Get());
            if (!result)
                ythrow TXPathError() << "cannot apply context xpath (incorrect expression?): " << expr;
            return result;
        }

        xmlNode* GetRoot() {
            return xmlDocGetRootElement(XmlDoc.Get());
        }
        void SetRoot(xmlNode* node) {
            xmlDocSetRootElement(XmlDoc.Get(), node);
        }
    };

    static int XmlWriteToOstream(void* context, const char* buffer, int len) {
        IOutputStream* out = (IOutputStream*)context;
        out->Write(buffer, len);
        return len;
    }

    TXmlDoc::TXmlDoc(const TArrayRef<const char>& doc) {
        Impl.Reset(new TImpl(doc));
    }

    TXmlDoc::TXmlDoc(const char* text, size_t len) {
        Impl.Reset(new TImpl(TArrayRef<const char>(text, len)));
    }

    TXmlDoc::TXmlDoc() {
        Impl.Reset(new TImpl());
    }

    TXmlDoc::~TXmlDoc() {
    }

    void TXmlDoc::Save(IOutputStream& to) {
        Impl->Save(to);
    }

    TxmlXPathObjectPtr TXmlDoc::Select(const TString& expr) {
        return Impl->Select(expr.data());
    }

    TxmlXPathObjectPtr TXmlDoc::Select(const TString& expr, const xmlNodePtr context) {
        return Impl->Select(expr.data(), context);
    }

    xmlNode* TXmlDoc::GetRoot() {
        return Impl->GetRoot();
    }

    void TXmlDoc::SetRoot(xmlNode* node) {
        Impl->SetRoot(node);
    }

}

template <>
void Out<xmlNodePtr>(IOutputStream& o, const xmlNodePtr node) {
    using namespace NXml;
    TxmlSaveCtxtHolder ctx;

    xmlSaveSetAttrEscape(ctx.Get(), nullptr);
    xmlSaveSetEscape(ctx.Get(), nullptr);

    ctx.Reset(xmlSaveToIO(XmlWriteToOstream, /* close */ nullptr, &o, /* enc = utf8 */ nullptr, XML_SAVE_FORMAT));
    xmlSaveTree(ctx.Get(), node);
}
