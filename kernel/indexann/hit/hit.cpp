#include "hit.h"

IOutputStream& NIndexAnn::operator<<(IOutputStream& out, const NIndexAnn::THit& hit) {
    out << "[" << hit.DocId() << "." << hit.Break() << "." << hit.Region() << "." << hit.Stream() << "." << hit.Value() << "]";
    return out;
}

