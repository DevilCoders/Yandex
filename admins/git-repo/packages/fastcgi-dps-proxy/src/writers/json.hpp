#ifndef JSONWRITER_HPP_INCLUDED
#define JSONWRITER_HPP_INCLUDED

#include "abstract.hpp"
#include <jansson.h>

using namespace std;

namespace DPS {
	namespace Writer {
		class JSONWriter : public AbstractWriter {
			public:
                JSONWriter(fastcgi::RequestStream* stream);  
                void writeItem(Entry& entry, bool& fetchContent);
                ~JSONWriter();
            private:
                json_t* root;
                json_t* items;
		};
	}
}

#endif
