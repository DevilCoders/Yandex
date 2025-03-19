#include "xml.hpp"

namespace DPS {
	namespace Writer {
        XMLWriter::XMLWriter(fastcgi::RequestStream* stream, Entry& root) {
            this->stream = stream;
            buf = xmlBufferCreate();
            libxmlWriter = xmlNewTextWriterMemory(buf, 0);
            xmlTextWriterStartDocument(libxmlWriter, NULL, "UTF-8", NULL);
            xmlTextWriterStartElement(libxmlWriter, BAD_CAST "container");
            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "name", BAD_CAST root.path.c_str());
        };

        XMLWriter::~XMLWriter() {
            xmlTextWriterEndElement(libxmlWriter);
            xmlTextWriterEndDocument(libxmlWriter);
            xmlFreeTextWriter(libxmlWriter);
            (*stream) << buf->content;
            xmlBufferFree(buf); 
        };

        void XMLWriter::writeItem(Entry& entry, bool& fetchContent) {
            xmlTextWriterStartElement(libxmlWriter, BAD_CAST "item");

            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "name", BAD_CAST entry.name.c_str());
            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "fullname", BAD_CAST entry.path.c_str());
            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "type", BAD_CAST entry.stype.c_str());

            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "id", BAD_CAST to_string(entry.id).c_str());
            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "creation_date", BAD_CAST entry.creationDate.c_str());
            xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "modification_date", BAD_CAST entry.modificationDate.c_str());

            if (entry.isDeleted) {
                xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "deleted", BAD_CAST "yes");
            }

            if (entry.IsFile() && entry.properties.size()) {
                xmlTextWriterStartElement(libxmlWriter, BAD_CAST "properties");
                for (Property prop : entry.properties) {
                    xmlTextWriterStartElement(libxmlWriter, BAD_CAST "property");
                    xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "name", BAD_CAST prop.name.c_str());
                    xmlTextWriterWriteAttribute(libxmlWriter, BAD_CAST "value", BAD_CAST prop.value.c_str());
                    xmlTextWriterEndElement(libxmlWriter);
                }
                xmlTextWriterEndElement(libxmlWriter);
            }

            if (entry.IsFile() && fetchContent) {
                xmlTextWriterStartElement(libxmlWriter, BAD_CAST "content");
                xmlTextWriterWriteBase64(libxmlWriter, entry.content.c_str(), 0, entry.content.length());
                xmlTextWriterEndElement(libxmlWriter);
            }

            xmlTextWriterEndElement(libxmlWriter);
		};
	}
}
