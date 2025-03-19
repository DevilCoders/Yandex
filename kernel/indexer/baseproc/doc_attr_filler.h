#pragma once

#include <kernel/segmentator/structs/segment_span.h>
#include <kernel/segnumerator/segnumerator.h>

class IDocumentDataInserter;

void Fill(const NSegm::TSegmentSpans& spans, IDocumentDataInserter* inserter);
void StoreSegmentatorSpans(const NSegm::TSegmentatorHandler<>& handler, IDocumentDataInserter& inserter);

