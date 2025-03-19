#include "msgpack.hpp"

using namespace std;

namespace DPS {
	namespace Writer {
        MsgPackWriter::MsgPackWriter(fastcgi::RequestStream* stream, size_t itemsCount) : buffer(), pk(&buffer), itemsCount(itemsCount) {
            this->stream = stream;
            pk.pack_array(itemsCount - 1);
        };

        MsgPackWriter::~MsgPackWriter() {
            (*stream) << string(buffer.data(), buffer.size());
        };

        void MsgPackWriter::writeItem(Entry& entry, bool& fetchContent) {
            pk.pack_map(6 + 
                    (entry.isDeleted ? 1 : 0) + 
                    ((entry.IsFile() && entry.properties.size()) ? 1 : 0) +
                    ((entry.IsFile() && fetchContent) ? 1 : 0));

            pk.pack(string("name"));
            pk.pack(entry.name);
            pk.pack(string("fullname"));
            pk.pack(entry.path);
            pk.pack(string("type"));
            pk.pack(entry.stype);
            pk.pack(string("id"));
            pk.pack(entry.id);
            pk.pack(string("creation_date"));
            pk.pack(entry.creationDate);
            pk.pack(string("modification_date"));
            pk.pack(entry.modificationDate);

            if (entry.isDeleted) {
                pk.pack(string("deleted"));
                pk.pack(string("yes"));
            }

            if (entry.IsFile() && entry.properties.size()) {
                pk.pack(string("properties"));
                pk.pack_map(entry.properties.size());
                for (Property prop : entry.properties) {
                    pk.pack(prop.name);
                    pk.pack(prop.value);
                } 
            }

            if (entry.IsFile() && fetchContent) {
                pk.pack(string("content"));
                pk.pack_raw(entry.content.size());
                pk.pack_raw_body(entry.content.c_str(), entry.content.size());
            }
		};
	}
}
