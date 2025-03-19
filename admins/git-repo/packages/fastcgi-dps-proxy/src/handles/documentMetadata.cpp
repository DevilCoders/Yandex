#include "documentMetadata.hpp"
#include <yandex/logger.h>
#include <chrono>

using namespace std;

namespace DPS {
    namespace Handle {

        DocumentMetadata::DocumentMetadata(fastcgi::Request *request, fastcgi::HandlerContext *context) {

            properties = true;
            revisions = true;

            file = request->getArg("file");

            auto propsArg = request->getArg("properties");
            transform(propsArg.begin(), propsArg.end(), propsArg.begin(), ::tolower);

            if (propsArg == "false" || propsArg == "no" || propsArg == "0")
                properties = false;

            auto revsArg = request->getArg("revisions");
            transform(revsArg.begin(), revsArg.end(), revsArg.begin(), ::tolower);

            if (revsArg == "false" || revsArg == "no" || revsArg == "0")
                revisions = false;

            req = request;
            ctx = context;

            process();
        };

        void DocumentMetadata::process() {
            MysqlLayer* mysql = &SingletonMysqlLayer::Instance();

            if (file.empty()) {
                req->setStatus(400);
                fastcgi::RequestStream stream(req);
                stream << "'file' parameter is required";                
                return;
            }

            req->setStatus(200);
            req->setContentType("text/xml; charset=utf-8");

            xmlBufferPtr buf = xmlBufferCreate();
            xmlTextWriterPtr libxmlWriter = xmlNewTextWriterMemory(buf, 0);
            xmlTextWriterStartDocument(libxmlWriter, NULL, "UTF-8", NULL);
            xmlTextWriterStartElement(libxmlWriter, BAD_CAST "document_metadata");
            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "name", BAD_CAST file.c_str());

            if (revisions) {
                Revisions revisions = mysql->GetRevisions(file);
                xmlTextWriterStartElement(libxmlWriter, BAD_CAST "revisions");
                for (Revision rev : revisions) {
                    xmlTextWriterStartElement(libxmlWriter, BAD_CAST "revision");
                    xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "number", BAD_CAST to_string(rev.id).c_str());
                    xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "date", BAD_CAST rev.creationDate.c_str());
                    xmlTextWriterEndElement(libxmlWriter);
                }
                xmlTextWriterEndElement(libxmlWriter);
            }

            if (properties) {
                Properties properties  = mysql->GetProperties(file);
                xmlTextWriterStartElement(libxmlWriter, BAD_CAST "properties");
                for (Property prop : properties) {
                    xmlTextWriterStartElement(libxmlWriter, BAD_CAST "property");
                    xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "name", BAD_CAST prop.name.c_str());
                    xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "value", BAD_CAST prop.value.c_str());
                    xmlTextWriterEndElement(libxmlWriter);
                }
                xmlTextWriterEndElement(libxmlWriter);
            }

            xmlTextWriterEndElement(libxmlWriter);
            xmlTextWriterEndDocument(libxmlWriter);
            xmlFreeTextWriter(libxmlWriter);
            fastcgi::RequestStream stream(req);
            stream << buf->content;
            xmlBufferFree(buf); 
                
        }	

    }
}
