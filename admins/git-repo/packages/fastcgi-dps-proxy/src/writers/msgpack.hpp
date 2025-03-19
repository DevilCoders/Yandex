#ifndef MsgPackWRITER_HPP_INCLUDED
#define MsgPackWRITER_HPP_INCLUDED

#include "abstract.hpp"
#include <msgpack.hpp>

using namespace std;

namespace DPS {
	namespace Writer {
		class MsgPackWriter : public AbstractWriter {
			public:
                MsgPackWriter(fastcgi::RequestStream* stream, size_t itemsCount);  
                void writeItem(Entry& entry, bool& fetchContent);
                ~MsgPackWriter();
            private:
                msgpack::sbuffer buffer;
                msgpack::packer<msgpack::sbuffer> pk;
                size_t itemsCount;
		};
	}
}

#endif
