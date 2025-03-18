#pragma once

/** @file yandex/xconfih.h
 * Header for XML base configuration utility
 * Contains three classes that form together simple representation of
 * the parsed XML configuration file and it's parts.
 * Parts in XML are located via XPathTStrings.
 */
#include <contrib/libs/libxml/include/libxml/parser.h>
#include <contrib/libs/libxml/include/libxml/tree.h>
#include <contrib/libs/libxml/include/libxml/xinclude.h>
#include <contrib/libs/libxml/include/libxml/xpath.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <errno.h>
#include <stdlib.h>

#ifndef BAD_REV_CAST
#define BAD_REV_CAST(a) reinterpret_cast<const char*>(a)
#endif

namespace NBlackbox2 {
    namespace xmlConfig {
        /** @var const long int BadLongVal = 0x7FFFFFFFL
 * @brief Denotes bad value whenTString is being converted to long.
 */
        const long int BadLongVal = 0x7FFFFFFFL;

        class Parts;
        class Part;

        /** @class XConfig
 * @brief XConfig handles XML document location and parsing.
 * XConfig is responsible for location of configuration file if the file wasn't
 * specified, for parsing of the XML and for access to parts of the parsed
 * tree via XPath expressions.
 * Provides access to Parts list and Part object.
 */
        class XConfig {
        public:
            /** @fn XConfig()
         * Default and only constructor.
         */
            XConfig()
                : doc(0)
            {
            }
            /** @fn ~XConfig()
         * Destructor cleans up after work with xml.
         */
            virtual ~XConfig();

            /** @fn Parse(const TString &xmltext)
         * Parses XML document represented by the specifiedTString.
         * @return 0 if theTString was successfuly parsed
         * @return ENOMEM if it's not sufficient memory for parsing
         * @return EDOM if parsing of the document failed
         * @note XIncludes are not processed.
         * @note Parser processes all documents with XML_PARSE_NOCDATA on.
         */
            inline int Parse(const TStringBuf xmltext) {
                return realparse(xmltext);
            }
            /** @fn GetParts(const char * xpathExpr)
         * Constructs list of the xml tree nodes matching specified XPath
         * expression and returns it as Parts object.
         * @return Parts object
         * @note Resulting object may be of zero size and this woun't rise
         * an exception.
         */
            Parts GetParts(const char* xpathExpr);
            /** @fn GetFirst(const char * xpathExpr)
         * Returns first node from the list of nodes matching XPath expression
         * @exception initexc if no nodes match specified XPath expression
         * @return Part object referencing first node matching XPath expression
         */
            Part GetFirst(const char* xpathExpr);
            /** @fn GetIfExists(const char * xpathExpr, long int & variable)
         * Tries to locate node via specified XPath and if found tries
         * to convert contents of the node to long. Stores obtained value
         * in passed variable.
         * @return true if variable was set, false otherwise.
         */
            bool GetIfExists(const char* xpathExpr, long int& variable);
            /** @fn GetIfExists(const char * xpathExpr, TString & variable)
         * Tries to locate node via specified XPath and if found
         * stores node contents in passed variable.
         * @return true if variable was modified, false otherwise.
         */
            bool GetIfExists(const char* xpathExpr, TString& variable);

        private:
            int realparse(const TStringBuf xmltext);
            xmlDocPtr doc;
        };

        /** @class Part
 * @brief Handler to node in XML tree
 * This class allows to build relative XPath expressions easily. All requests
 * for Parst or Part objects with XPath expressions not starting with '/' will
 * be treated relative to the node given Part object is linked to.
 * Part object cna be obtained through call to GetFirst or by indexing Parts
 * object via operator[].
 * @see Parts
 * @see XConfig
 */
        class Part {
            friend class Parts;
            friend class XConfig;

        private:
            xmlDocPtr doc;
            xmlNodePtr thisnode;

        public:
            /** @fn Part(const Part & rhs)
         * @brief Copy constructor
         * Resulting Part object refers to the same xml tree node as origin.
         */
            Part(const Part& rhs);
            /** @fn GetParts(const char * xpathExpr)
         * Constructs list of the xml tree nodes matching specified XPath
         * expression and returns it as Parts object.
         * @return Parts object
         * @note Resulting object may be of zero size and this woun't rise
         * an exception. XPath expressions in this case may be relative.
         */
            Parts GetParts(const char* xpathExpr);
            /** @fn GetFirst(const char * xpathExpr)
         * Returns first node from the list of nodes matching XPath expression
         * @exception initexc if no nodes match specified XPath expression
         * @return Part object referencing first node matching XPath expression
         * @note XPath expressions in this case may be relative.
         */
            Part GetFirst(const char* xpathExpr);
            /** @fn TString GetName()
         * @brief Return the name of the node asTString.
         * Returns the name of the xml tree node this instance if refering
         * as TString.
         */
            TString GetName();
            /** @fn TString asString()
         * @brief Return contents of the node asTString.
         * Returns contents of the xml tree node this instance if refering
         * as TString. All text (CDATA) contents of the node is merged and
         * entities are substituted.
         */
            TString asString();
            /** @fn long int asLong(long int defval = BadLongVal)
         * @brief Returns decimal representation for the current node contents
         * Tries to convert contents of the xml tree node into long int number.
         * Accepts single parameter of the long int type. If conversion failed
         * for some reason the value passed will be returned. By default this
         * value is #BadLongVal.
         */
            long int asLong(long int defval = BadLongVal);
            /** @fn GetIfExists(const char * xpathExpr, long int & variable)
         * Tries to locate node via specified XPath and if found tries
         * to convert contents of the node to long. Stores obtained value
         * in passed variable.
         * @return true if variable was set, false otherwise.
         */
            bool GetIfExists(const char* xpathExpr, long int& variable);
            /** @fn GetIfExists(const char * xpathExpr, TString & variable)
         * Tries to locate node via specified XPath and if found
         * stores node contents in passed variable.
         * @return true if variable was modified, false otherwise.
         */
            bool GetIfExists(const char* xpathExpr, TString& variable);
#ifdef DEBUG
            int printcontent();
#endif
        private:
            Part(xmlDocPtr doc_, xmlNodePtr node);

        private:
            Part();
        };

        /** @class Parts
 * @brief wrapper for xml tree nodeset.
 * This class is used to store and access the list of the xml tree nodes
 * obtaied as set of nodes matched by XPath expression via call to
 * GetParts(const char * xpathExpr).
 * Supports acces to separate node via GetFirst and operator[].
 */
        class Parts {
            friend class XConfig;
            friend class Part;

        public:
            /** @fn ~Parts()
         * Frees xml nodeset
         */
            ~Parts();
            /** @fn Parts(const Parts & rhs)
         * @brief Copy constructor.
         * This copy constructor is so called ownership thief. It is made to
         * return Parts object without deep xml tree structure copying.
         * @note Right hande side object is empty (unusable) after copying.
         */
            Parts(const Parts& rhs);
            /** @fn operator[](int number)
         * @brief Indexing accessor to the xml nodeset.
         * @return Node from set by its index starting from 0 as Part object.
         * @note No checking on index bounds is made.
         */
            Part operator[](int number);
            /** @fn GetFirst()
         * Returns first node from the nodeset refered by this Parts instance.
         * @exception initexc if this instance has zero size.
         * @return Part object referencing first node in nodeset.
         */
            Part GetFirst();
            /** @fn Size()
         * @return Size of the nodeset refered by given Parts instance.
         */
            int Size() {
                return size;
            }

        private:
            xmlDocPtr doc;
            mutable xmlXPathContextPtr xpathCtx;
            mutable xmlXPathObjectPtr xpathObj;
            int size;

        private:
            Parts(xmlDocPtr doc, const char* xpathExpr);
            Parts(xmlDocPtr doc, xmlNodePtr startnode, const char* xpathExpr);
            int evaluate(const char* xpathExpr);
            int setcurnode(xmlNodePtr node);

        private:
            Parts();
            Parts& operator=(const Parts& rhs);
        };

    }
};    // namespace NBlackbox2
