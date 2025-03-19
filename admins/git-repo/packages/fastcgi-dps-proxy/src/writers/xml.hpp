#ifndef XMLWRITER_HPP_INCLUDED
#define XMLWRITER_HPP_INCLUDED

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include "abstract.hpp"

using namespace std;

namespace DPS {
	namespace Writer {
		class XMLWriter : public AbstractWriter {
			public:
                XMLWriter(fastcgi::RequestStream* stream, Entry& root);  
                void writeItem(Entry& entry, bool& fetchContent);
                ~XMLWriter();
            private:
                xmlBufferPtr buf;
                xmlTextWriterPtr libxmlWriter;
		};
	}
}

#endif
