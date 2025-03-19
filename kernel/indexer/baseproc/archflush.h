#pragma once

#include <util/generic/ptr.h>

struct TDocInfoEx;
class TFullDocAttrs;
struct TArchiveProcessorConfig;
class IOutputStream;
class TFullArchiveWriter;

class TFullArchiveFlusher {
public:
    TFullArchiveFlusher(const TArchiveProcessorConfig* cfg);
    TFullArchiveFlusher(IOutputStream* fullArchive, bool writeHeader = false); // stream is not owned by flusher
    ~TFullArchiveFlusher();

    void FlushDoc(const TDocInfoEx* docInfo, const TFullDocAttrs& docAttrs);
    void Term();

private:
    void Init(bool writeHeader);

    IOutputStream* FullArchive;
    THolder<IOutputStream> FullArchiveHolder;
    THolder<TFullArchiveWriter> FullArchiveWriter;
};
