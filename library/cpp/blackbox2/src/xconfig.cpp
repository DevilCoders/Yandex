#include "xconfig.h"

#include "initexc.h"

#include <util/stream/str.h>

namespace NBlackbox2 {
    namespace xmlConfig {
        XConfig::~XConfig() {
            if (0 != doc) {
                xmlFreeDoc(doc);
            }
        }

        int
        XConfig::realparse(const TStringBuf xmltext) {
            doc = xmlReadMemory(xmltext.data(), static_cast<int>(xmltext.size()), NULL, NULL, XML_PARSE_NOCDATA);
            if (0 == doc) {
                return EDOM;
            }
            return 0;
        }

        Parts
        XConfig::GetParts(const char* xpathExpr) {
            return Parts(doc, xpathExpr);
        }

        Part
        XConfig::GetFirst(const char* xpathExpr) {
            Parts parts(doc, xpathExpr);
            if (0 == parts.Size()) {
                throw TInitExc() << "in GetFirst no parts for </+" << xpathExpr << ">";
            }
            return Part(doc, parts.xpathObj->nodesetval->nodeTab[0]);
        }

        bool
        XConfig::GetIfExists(const char* xpathExpr, long int& variable) {
            Parts parts(doc, xpathExpr);
            if (0 == parts.Size()) {
                return false;
            }
            xmlChar* ctp = xmlNodeGetContent(parts.xpathObj->nodesetval->nodeTab[0]);
            if (0 == ctp)
                return false;
            variable = strtol(BAD_REV_CAST(ctp), 0, 0);
            xmlFree(ctp);
            return true;
        }

        bool
        XConfig::GetIfExists(const char* xpathExpr, TString& variable) {
            Parts parts(doc, xpathExpr);
            if (0 == parts.Size()) {
                return false;
            }
            xmlChar* ctp = xmlNodeGetContent(parts.xpathObj->nodesetval->nodeTab[0]);
            if (0 == ctp) {
                variable.resize(0);
            } else {
                variable.assign(BAD_REV_CAST(ctp));
            }
            xmlFree(ctp);
            return true;
        }

        Parts::Parts(xmlDocPtr doc_, const char* xpathExpr)
            : doc(doc_)
            , xpathCtx(xmlXPathNewContext(doc))
            , xpathObj(0)
            , size(0)
        {
            if (0 == xpathCtx) {
                //E("Can't initialize context");
            }
            evaluate(xpathExpr);
        }

        Parts::Parts(xmlDocPtr doc_, xmlNodePtr startnode, const char* xpathExpr)
            : doc(doc_)
            , xpathCtx(xmlXPathNewContext(doc))
            , xpathObj(0)
            , size(0)
        {
            if (0 == xpathCtx) {
                //E("Can't initialize context");
            }
            setcurnode(startnode);
            evaluate(xpathExpr);
        }

        int
        Parts::setcurnode(xmlNodePtr node) {
            xpathCtx->node = node;
            return 0;
        }

        /*
 * Copy ctor optimized to return Parts instances by value from functions.
 * It modifies right hand side expression so dtor doesn't delete xml tree
 * parts we are still working with. Thus Parts instances steal ownership of
 * underlying xml tree objects when copy constructed.
 * If we want to perform honest copying we must copy xml tree parts as well,
 * which is unnecessary in our case.
 */
        Parts::Parts(const Parts& rhs)
            : doc(rhs.doc)
            , xpathCtx(rhs.xpathCtx)
            , xpathObj(rhs.xpathObj)
            , size(rhs.size)
        {
            //Parts & parts = const_cast<Parts &>(rhs);
            const Parts& parts = rhs;
            parts.xpathCtx = 0;
            parts.xpathObj = 0;
        }

        Parts::~Parts() {
            if (0 != xpathObj)
                xmlXPathFreeObject(xpathObj);
            if (0 != xpathCtx)
                xmlXPathFreeContext(xpathCtx);
        }

        int
        Parts::evaluate(const char* xpathExpr) {
            xpathObj = xmlXPathEvalExpression(reinterpret_cast<xmlChar*>(const_cast<char*>(xpathExpr)), xpathCtx);
            if (0 == xpathObj) {
                //E("Can't eval xpath expression" << xpathExpr);
                return EDOM;
            }
            xmlNodeSetPtr nodes = xpathObj->nodesetval;
            size = (nodes) ? nodes->nodeNr : 0;
            return 0;
        }

        Part
            Parts::operator[](int number) {
            return Part(doc, xpathObj->nodesetval->nodeTab[number]);
        }

        Part::Part(const Part& rhs)
            : doc(rhs.doc)
            , thisnode(rhs.thisnode)
        {
        }

        Part::Part(xmlDocPtr doc_, xmlNodePtr node)
            : doc(doc_)
            , thisnode(node)
        {
        }

        Parts
        Part::GetParts(const char* xpathExpr) {
            return Parts(doc, thisnode, xpathExpr);
        }

        Part
        Part::GetFirst(const char* xpathExpr) {
            Parts parts(doc, thisnode, xpathExpr);
            if (0 == parts.Size()) {
                TStringStream ss_;
                ss_ << "in GetFirst no parts for <";
                xmlChar* path = xmlGetNodePath(thisnode);
                if (0 != path) {
                    ss_ << "+" << BAD_REV_CAST(path);
                    xmlFree(path);
                }
                ss_ << xpathExpr << ">";
                throw TInitExc() << ss_.Str();
            }
            return Part(doc, parts.xpathObj->nodesetval->nodeTab[0]);
        }

        bool
        Part::GetIfExists(const char* xpathExpr, long int& variable) {
            Parts parts(doc, thisnode, xpathExpr);
            if (0 == parts.Size()) {
                return false;
            }
            xmlChar* ctp = xmlNodeGetContent(parts.xpathObj->nodesetval->nodeTab[0]);
            if (0 == ctp)
                return false;
            variable = strtol(BAD_REV_CAST(ctp), 0, 0);
            xmlFree(ctp);
            return true;
        }

        bool
        Part::GetIfExists(const char* xpathExpr, TString& variable) {
            Parts parts(doc, thisnode, xpathExpr);
            if (0 == parts.Size()) {
                return false;
            }
            xmlChar* ctp = xmlNodeGetContent(parts.xpathObj->nodesetval->nodeTab[0]);
            if (0 == ctp) {
                variable.resize(0);
            } else {
                variable.assign(BAD_REV_CAST(ctp));
            }
            xmlFree(ctp);
            return true;
        }

        TString
        Part::GetName() {
            TString aux(BAD_REV_CAST(thisnode->name));
            return aux;
        }

        TString
        Part::asString() {
            xmlChar* ctp = xmlNodeGetContent(thisnode);
            if (0 == ctp)
                return TString();
            TString aux(BAD_REV_CAST(ctp));
            xmlFree(ctp);
            return aux;
        }

        long int
        Part::asLong(long int defval) {
            xmlChar* ctp = xmlNodeGetContent(thisnode);
            if (0 == ctp)
                return defval;
            long int aux = strtol(BAD_REV_CAST(ctp), 0, 0);
            xmlFree(ctp);
            return aux;
        }

    }
};    // namespace NBlackbox2
