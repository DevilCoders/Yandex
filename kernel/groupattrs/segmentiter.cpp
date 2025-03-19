#include "segmentiter.h"

#include "categseries.h"
#include "docsattrs.h"

namespace NGroupingAttrs {

void TSegments::Init(const TDocsAttrs& da, const TVector<ui32>& attrnums) {
    TCategSeries result;
    for (ui32 docid = 0; docid < da.DocCount(); ++docid) {
        for (size_t i = 0; i < attrnums.size(); ++i) {
            result.Clear();
            da.DocCategs(docid, attrnums[i], result);
            for (const TCateg* c = result.Begin(); c != result.End(); ++c) {
                Data[*c].VisitDoc(docid);
            }
        }
    }

    for (TData::iterator it = Data.begin() ; it != Data.end() ; ++it) {
        it->second.Shrink();
    }
}

}
