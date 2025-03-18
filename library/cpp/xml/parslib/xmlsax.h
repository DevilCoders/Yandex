#pragma once

#include <library/cpp/html/entity/htmlentity.h>

#include <util/generic/buffer.h>
#include <library/cpp/charset/codepage.h>
#include <util/charset/utf8.h>
#include <util/generic/yexception.h>

#include <libxml/parser.h>

class TXmlHandlerBase {
protected:
    void OnDocumentStart() {
    }
    void OnDocumentEnd() {
    }
    void OnElementStart(const xmlChar* /*name*/, const xmlChar** /*attrs*/) {
    }
    void OnElementEnd(const xmlChar* /*name*/) {
    }
    void OnText(const xmlChar* /*text*/, int /*len*/) {
    }

protected:
    TXmlHandlerBase() {
    }

private:
    TXmlHandlerBase(const TXmlHandlerBase&);
    TXmlHandlerBase& operator=(const TXmlHandlerBase&);
};

template <class Handler>
class TXmlHandlerWrapper: public Handler {
public:
    void AllowHtmlEntities(bool allow) {
        HtmlEntitiesAllowed = allow;
    }

protected:
//-----
#define ME static_cast<TXmlHandlerWrapper*>(me)
    //-----

    static void StDocumentStart(void* me) {
        ME->OnDocumentStart();
    }

    static void StDocumentEnd(void* me) {
        ME->OnDocumentEnd();
    }

    static void StElementStart(void* me, const xmlChar* name, const xmlChar** attrs) {
        ME->OnElementStart(name, attrs);
    }

    static void StElementEnd(void* me, const xmlChar* name) {
        ME->OnElementEnd(name);
    }

    static void StText(void* me, const xmlChar* text, int len) {
        ME->OnText(text, len);
    }

    static xmlEntityPtr StEntity(void* me, const xmlChar* name) {
        return ME->OnEntity(name);
    }

//-----
#undef ME
    //-----

protected:
    TXmlHandlerWrapper()
        : Handler()
    {
        memset(&Entity, 0, sizeof(Entity));
        EntityText[0] = '\0';
        Entity.content = EntityText;
    }

    xmlEntityPtr OnEntity(const xmlChar* name) {
        xmlEntityPtr ret = xmlGetPredefinedEntity(name);
        ret = ret ? ret : (HtmlEntitiesAllowed ? DecodeHtmlEntity(name) : nullptr);
        return ret;
    }

    static const xmlSAXHandler sax;

private:
    xmlEntityPtr DecodeHtmlEntity(const xmlChar* name) {
        const char* entityName = (const char*)name;
        size_t entityLen = strlen(entityName);
        TBuffer buffer(entityLen + 1);
        buffer.Assign(entityName, entityLen);
        buffer.Append(';');
        TEntity entity;
        if (DecodeNamedEntity((const unsigned char*)buffer.Data(), buffer.Size(), &entity) && entity.Codepoint2 == 0) {
            size_t len = 0;
            SafeWriteUTF8Char(entity.Codepoint1, len, EntityText, EntityText + sizeof(EntityText) - 1);
            EntityText[len] = '\0';
            return &Entity;
        }
        return nullptr;
    }

    bool HtmlEntitiesAllowed;
    xmlEntity Entity;
    xmlChar EntityText[7];

private:
    TXmlHandlerWrapper(const TXmlHandlerWrapper&);
    TXmlHandlerWrapper& operator=(const TXmlHandlerWrapper&);
};

template <class Handler>
const xmlSAXHandler TXmlHandlerWrapper<Handler>::sax = {
    nullptr,         // internalSubsetSAXFunc        internalSubset
    nullptr,         // isStandaloneSAXFunc          isStandalone
    nullptr,         // hasInternalSubsetSAXFunc     hasInternalSubset
    nullptr,         // hasExternalSubsetSAXFunc     hasExternalSubset
    nullptr,         // resolveEntitySAXFunc         resolveEntity
    StEntity,        // getEntitySAXFunc             getEntity
    nullptr,         // entityDeclSAXFunc            entityDecl
    nullptr,         // notationDeclSAXFunc          notationDecl
    nullptr,         // attributeDeclSAXFunc         attributeDecl
    nullptr,         // elementDeclSAXFunc           elementDecl
    nullptr,         // unparsedEntityDeclSAXFunc    unparsedEntityDecl
    nullptr,         // setDocumentLocatorSAXFunc    setDocumentLocator
    StDocumentStart, // startDocumentSAXFunc         startDocument
    StDocumentEnd,   // endDocumentSAXFunc           endDocument
    StElementStart,  // startElementSAXFunc          startElement
    StElementEnd,    // endElementSAXFunc            endElement
    nullptr,         // referenceSAXFunc             reference
    StText,          // charactersSAXFunc            characters
    nullptr,         // ignorableWhitespaceSAXFunc   ignorableWhitespace
    nullptr,         // processingInstructionSAXFunc processingInstruction
    nullptr,         // commentSAXFunc               comment
    nullptr,         // warningSAXFunc               warning
    nullptr,         // errorSAXFunc                 error
    nullptr,         // fatalErrorSAXFunc            fatalError
    nullptr,         // getParameterEntitySAXFunc    getParameterEntity
    nullptr,         // cdataBlockSAXFunc            cdataBlock
    nullptr,         // externalSubsetSAXFunc        externalSubset
    0,               // unsigned int                 initialized
    nullptr,         // void *                       _private
    nullptr,         // startElementNsSAX2Func       startElementNs
    nullptr,         // endElementNsSAX2Func         endElementNs
    nullptr,         // xmlStructuredErrorFunc       serror
};

class TXmlSaxException: public yexception {
};

template <class Handler = TXmlHandlerBase>
class TCustomContextXmlSaxParserImpl: public TXmlHandlerWrapper<Handler> {
};

template <class Handler>
class TDefaultContextXmlSaxParserImpl: public TXmlHandlerWrapper<Handler> {
protected:
    TDefaultContextXmlSaxParserImpl()
        : Context(nullptr)
    {
    }

    ~TDefaultContextXmlSaxParserImpl() {
        if (Context) {
            xmlFreeParserCtxt(Context);
            Context = nullptr;
        }
    }

    xmlParserCtxtPtr Context;
};

template <class Handler = TXmlHandlerBase, template <class> class Impl = TDefaultContextXmlSaxParserImpl>
class TXmlSaxParser: public Impl<Handler> {
    using Impl<Handler>::TXmlHandlerWrapper::sax;
    using Impl<Handler>::TXmlHandlerWrapper::AllowHtmlEntities;
    using Impl<Handler>::Context;

public:
    TXmlSaxParser()
        : Impl<Handler>()
    {
    }

    ~TXmlSaxParser() {
        if (Context) {
            xmlFreeParserCtxt(Context);
            Context = nullptr;
        }
    }

    void Start(bool allowHtmlEntities = false, bool silentMode = false) {
        Context = xmlCreatePushParserCtxt(const_cast<xmlSAXHandlerPtr>(&sax),
                                          static_cast<TXmlHandlerWrapper<Handler>*>(this),
                                          nullptr, 0, nullptr);
        if (!Context)
            ythrow TXmlSaxException() << "can't create XML parser: " << DescribeError(silentMode);
        xmlCtxtUseOptions(Context, XML_PARSE_NOENT);
        AllowHtmlEntities(allowHtmlEntities);
    }

    void Parse(const char* data, const size_t len, bool silentMode = false) {
        int err = xmlParseChunk(Context, data, len, 0);
        if (err)
            ythrow TXmlSaxException() << "can't parse: err=" << err << ": " << DescribeError(silentMode) << "";
    }

    void Final(bool silentMode = false) {
        int err = xmlParseChunk(Context, nullptr, 0, 1);
        if (err)
            ythrow TXmlSaxException() << "can't finalize parser: err=" << err << ": " << DescribeError(silentMode) << "";
        xmlFreeParserCtxt(Context);
        Context = nullptr;
    }

private:
    const char* DescribeError(bool silentMode) {
        xmlErrorPtr err = xmlGetLastError();
        if (!err)
            return "no error";
        if (!silentMode)
            xmlParserError(Context, "invalid xml\n");
        return err->message;
    }
};
