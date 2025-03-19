#include "json.hpp"
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/binary_from_base64.hpp>
#include <boost/archive/iterators/transform_width.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/remove_whitespace.hpp>

using namespace boost::archive::iterators;
using namespace std;

// If you want linebreaks:
// typedef insert_linebreaks<base64_from_binary<transform_width<string::const_iterator,6,8> >, 72 > it_base64_t;

typedef base64_from_binary<transform_width<string::const_iterator,6,8> > it_base64_t;

namespace DPS {
	namespace Writer {
        JSONWriter::JSONWriter(fastcgi::RequestStream* stream) {
            this->stream = stream;
            this->root = json_object();
            this->items = json_array();
            json_object_set_new(this->root, "items", this->items);
        };

        JSONWriter::~JSONWriter() {
//            (*stream) << json_dumps(this->root, JSON_COMPACT);
            char* jsonOut=json_dumps(this->root, JSON_COMPACT);
            (*stream) << jsonOut;
            free(jsonOut);
            json_object_clear(this->root);
            json_decref(this->root);
        };

        void JSONWriter::writeItem(Entry& entry, bool& fetchContent) {
            json_t* item = json_pack("{s:s, s:s, s:s, s:i, s:s, s:s}", 
                        "name", entry.name.c_str(), 
                        "fullname", entry.path.c_str(), 
                        "type", entry.stype.c_str(), 
                        "id", entry.id, 
                        "creation_date", entry.creationDate.c_str(),
                        "modification_date", entry.modificationDate.c_str());

            if (entry.isDeleted) {
                json_object_set_new(item, "deleted", json_string("yes"));
            }

            if (entry.IsFile() && entry.properties.size()) {
                json_t* props = json_object();
                for (Property prop : entry.properties) {
                    json_object_set_new(props, prop.name.c_str(), json_string(prop.value.c_str()));
                } 
                json_object_set_new(item, "properties", props);
            }

            if (entry.IsFile() && fetchContent) {
                unsigned int writePaddChars = (3 - entry.content.length()%3)%3;
                string base64(it_base64_t(entry.content.begin()),it_base64_t(entry.content.end()));
                base64.append(writePaddChars, '=');
                json_object_set_new(item, "content", json_string(base64.c_str()));
            }
            
            json_array_append_new(this->items, item);
		};
	}
}
