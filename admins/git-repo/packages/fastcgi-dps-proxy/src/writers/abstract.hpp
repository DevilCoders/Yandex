#ifndef ABSTRACTWRITER_HPP_INCLUDED
#define ABSTRACTWRITER_HPP_INCLUDED

#include <fastcgi2/handler.h>
#include <fastcgi2/request.h>
#include <fastcgi2/stream.h>
#include "../types.hpp"

using namespace std;

namespace DPS {
	namespace Writer {
		class AbstractWriter {
			public:
                virtual void writeItem(Entry& entry, bool& fetchContent) = 0;
                fastcgi::RequestStream* stream;
                virtual ~AbstractWriter() { };
		};

	}
}

#endif
