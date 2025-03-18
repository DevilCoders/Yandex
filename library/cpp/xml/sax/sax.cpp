#include "sax.h"

#include <libxml/xmlIO.h>
#include <libxml/tree.h>

#include <library/cpp/xml/init/init.h>

#include <util/stream/input.h>
#include <util/string/printf.h>
#include <util/string/strip.h>
#include <util/generic/ptr.h>
#include <util/generic/yexception.h>
#include <util/generic/hash.h>
#include <util/generic/scope.h>

#include <libxml/parserInternals.h>

using namespace NXml;

namespace {
    struct TInit {
        inline TInit() {
            InitEngine();
        }
    } initer;

    struct TSaxEntity {
        const TString Value;
        xmlEntity Entity;

        TSaxEntity(const TString& value)
            : Value(value)
        {
            Zero(Entity);
            Entity.content = (xmlChar*)Value.data();
            Entity.orig = (xmlChar*)Value.data();
            Entity.length = (int)Value.size();
            Entity.type = XML_ENTITY_DECL;
            Entity.etype = XML_INTERNAL_GENERAL_ENTITY;
        }
    };

    struct TSaxHandler {
        THashMap<TString, TSaxEntity> Entities;

        inline TSaxHandler(ISaxHandler* sax) noexcept
            : ParserCtxtPtr(nullptr)
            , Sax(sax)
        {
        }

        inline xmlParserCtxtPtr GetParserCtxtPtr() noexcept {
            return ParserCtxtPtr;
        }

        inline void SetParserCtxtPtr(xmlParserCtxtPtr parserCtxtPtr) noexcept {
            ParserCtxtPtr = parserCtxtPtr;
        }

        inline xmlEntityPtr GetEntity(const char* entity) noexcept {
            TStringBuf entityBuf(entity);
            TString expansion;
            if (!Sax->GetEntity(entity, expansion)) {
                return nullptr;
            }
            const auto& ii = Entities.find(expansion);
            if (ii != Entities.end()) {
                return &ii->second.Entity;
            }
            return &Entities.emplace(expansion, expansion).first->second.Entity;
        }

        xmlParserCtxtPtr ParserCtxtPtr;
        ISaxHandler* Sax;
    };

    void Deallocate(xmlParserInputBuffer* source) {
        xmlFreeParserInputBuffer(source);
    }

    void Deallocate(xmlParserCtxt* ctxt) {
        if (ctxt->myDoc) {
            xmlFreeDoc(ctxt->myDoc);
            ctxt->myDoc = nullptr;
        }

        if (ctxt->input) {
            //deleted some time ago
            ctxt->input->buf = nullptr;
            xmlFreeInputStream(ctxt->input);
            ctxt->input = nullptr;
        }

        xmlFreeParserCtxt(ctxt);
    }

    struct TReleaseSource {
        static inline void Destroy(xmlParserInputBuffer* x) {
            Deallocate(x);
        }
    };

    struct TReleaseContext {
        static inline void Destroy(xmlParserCtxt* x) {
            Deallocate(x);
        }
    };

    typedef TAutoPtr<xmlParserInputBuffer, TReleaseSource> TSourcePtr;
    typedef TAutoPtr<xmlParserCtxt, TReleaseContext> TContextPtr;

    int ReadCallback(void* context, char* buffer, int len) {
        return ((IInputStream*)context)->Read(buffer, len);
    }

    int CloseCallback(void*) {
        return 0;
    }

    TSourcePtr CreateSource(IInputStream* input) {
        xmlParserInputBuffer* ret = xmlParserInputBufferCreateIO(ReadCallback, CloseCallback, (void*)input, XML_CHAR_ENCODING_NONE);

        if (!ret) {
            ythrow ISaxHandler::TError() << "can not allocate xml source";
        }

        return ret;
    }

    TContextPtr CreateContext(xmlParserInputBuffer* source) {
        TContextPtr ret(xmlNewParserCtxt());

        if (!ret) {
            ythrow ISaxHandler::TError() << "can not allocate xml context";
        }

        xmlParserInput* input = xmlNewIOInputStream(ret.Get(), source, XML_CHAR_ENCODING_NONE);

        if (!input) {
            ythrow ISaxHandler::TError() << "can not allocate xml input";
        }

        ret->input = input;

        return ret;
    }

    void StartDocument(void* ctx) {
        ((TSaxHandler*)(ctx))->Sax->OnStartDocument();
    }

    void EndDocument(void* ctx) {
        ((TSaxHandler*)(ctx))->Sax->OnEndDocument();
    }

    void StartElement(void* ctx, const xmlChar* name, const xmlChar** attrs) {
        ((TSaxHandler*)(ctx))->Sax->OnStartElement((const char*)name, (const char**)attrs);
    }

    void EndElement(void* ctx, const xmlChar* name) {
        ((TSaxHandler*)(ctx))->Sax->OnEndElement((const char*)name);
    }

    void Characters(void* ctx, const xmlChar* ch, int len) {
        ((TSaxHandler*)(ctx))->Sax->OnText((const char*)ch, len);
    }

    xmlEntityPtr GetEntity(void* ctx, const xmlChar* entity) {
        return ((TSaxHandler*)(ctx))->GetEntity((const char*)entity);
    }

    void GetEventContext(xmlParserCtxtPtr ctxt, size_t& line, size_t& column, TString& textContext) {
        if (!ctxt) {
            return;
        }

        xmlParserInputPtr input = ctxt->input;
        if ((input != nullptr) && (input->filename == nullptr) && (ctxt->inputNr > 1)) {
            input = ctxt->inputTab[ctxt->inputNr - 2];
        }

        line = (size_t)input->line;
        column = (size_t)input->col;

        if (!input->cur || !input->base || !input->end) {
            return;
        }

        size_t off = input->cur - input->base;
        size_t count = Min(off, (size_t)50);
        textContext = TString((const char*)input->base + off - count, count);
    }

    struct TParserContext: public IParserContext {
        TParserContext(xmlParserCtxtPtr ctxt, ISaxHandler* saxHandler)
            : Ctxt(ctxt)
            , SaxHandlerPtr(saxHandler)
        {
            SaxHandlerPtr->SetParserContext(this);
        }

        void GetContext(size_t& line, size_t& column, TString& textContext) override {
            GetEventContext(Ctxt, line, column, textContext);
        }

        virtual ~TParserContext() {
            SaxHandlerPtr->SetParserContext(nullptr);
        }

    private:
        xmlParserCtxtPtr Ctxt;
        ISaxHandler* SaxHandlerPtr;
    };

#define FORMAT_ERROR()                                                                          \
    va_list lst;                                                                                \
    va_start(lst, fmt);                                                                         \
    TString s;                                                                                  \
    vsprintf(s, fmt, lst);                                                                      \
    StripInPlace(s);                                                                            \
    ISaxHandler::TErrorInfo ei;                                                                 \
    ei.Message = s;                                                                             \
    GetEventContext(((TSaxHandler*)(ctx))->GetParserCtxtPtr(), ei.Line, ei.Column, ei.Context); \
    va_end(lst);

    void Warning(void* ctx, const char* fmt, ...) {
        FORMAT_ERROR();
        ((TSaxHandler*)(ctx))->Sax->OnWarning(ei);
    }

    void Error(void* ctx, const char* fmt, ...) {
        FORMAT_ERROR();
        ((TSaxHandler*)(ctx))->Sax->OnError(ei);
    }

    void FatalError(void* ctx, const char* fmt, ...) {
        FORMAT_ERROR();
        ((TSaxHandler*)(ctx))->Sax->OnFatalError(ei);
    }
}

ISaxHandler::ISaxHandler() noexcept {
}

ISaxHandler::~ISaxHandler() {
}

void ISaxHandler::OnWarning(const TErrorInfo& ei) {
    throw TParseError(ei);
}

void ISaxHandler::OnError(const TErrorInfo& ei) {
    throw TParseError(ei);
}

void ISaxHandler::OnFatalError(const TErrorInfo& ei) {
    throw TParseError(ei);
}

void ISaxHandler::OnStartDocument() {
}

void ISaxHandler::OnEndDocument() {
}

bool ISaxHandler::GetEntity(const TStringBuf& entityName, TString& entityExpansion) {
    Y_UNUSED(entityName);
    Y_UNUSED(entityExpansion);
    return false;
}

IParserContext* ISaxHandler::GetParserContext() {
    return Context;
}

void ISaxHandler::SetParserContext(IParserContext* pc) {
    Context = pc;
}

void NXml::ParseXml(IInputStream* input, ISaxHandler* saxh) {
    TSaxHandler hndl(saxh);
    TSourcePtr source(CreateSource(input));
    TContextPtr context(CreateContext(source.Get()));
    TParserContext parserContext(context.Get(), saxh);

    xmlSAXHandler* oldSax = context->sax;
    xmlSAXHandler sh;

    Zero(sh);

    sh.getEntity = GetEntity;
    sh.startDocument = StartDocument;
    sh.endDocument = EndDocument;
    sh.startElement = StartElement;
    sh.endElement = EndElement;
    sh.characters = Characters;
    sh.warning = Warning;
    sh.error = Error;
    sh.fatalError = FatalError;

    hndl.SetParserCtxtPtr(context.Get());

    context->sax = &sh;
    context->userData = &hndl;

    xmlCtxtUseOptions(context.Get(), XML_PARSE_NOENT | XML_PARSE_HUGE);

    Y_DEFER {
        context->sax = oldSax;
    };

    xmlParseDocument(context.Get());
}
