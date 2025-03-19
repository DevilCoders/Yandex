#include "multipart_archive.h"

template<>
TBlob FromArchive<TMultipartArchivePtr>(const TMultipartArchivePtr& arc, ui64 docId, ui64 dataOffset, size_t length) {
    TBlob doc = arc->GetDocument(docId);
    return doc.SubBlob(dataOffset, dataOffset + length).DeepCopy();
}

template<>
IArchive::TFactory::TRegistrator<TMemoryMultipartArchive> TMemoryMultipartArchive::Registrator({AT_MULTIPART, AOM_BLOB});
template<>
IArchive::TFactory::TRegistrator<TMapMultipartArchive> TMapMultipartArchive::Registrator({AT_MULTIPART, AOM_MAP});
template<>
IArchive::TFactory::TRegistrator<TFileMultipartArchive> TFileMultipartArchive::Registrator({AT_MULTIPART, AOM_FILE});
template<>
IArchive::TFactory::TRegistrator<TDirectFileMultipartArchive> TDirectFileMultipartArchive::Registrator({AT_MULTIPART, AOM_DIRECT_FILE});
