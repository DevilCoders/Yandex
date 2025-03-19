#pragma once

#include <kernel/remorph/facts/fact.h>

#include <kernel/fio_extractor/fiowordsequence.h>

namespace NFioExtractor {

    void Fact2FullFio(const NFact::TFact& fact, TFullFIO& fullFio, TFIOOccurence& fioOcc);

}
