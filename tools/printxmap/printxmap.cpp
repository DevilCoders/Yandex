// Распечатка информации, сохранённой в xmap-файле

#include <kernel/xref/xref_types.h>

#include <library/cpp/on_disk/2d_array/array2d.h>

#include <util/stream/output.h>
#include <util/stream/output.h>
#include <util/system/defaults.h>

int main(int argc, char** argv)
{
    if (argc != 2) {
        Cerr << "Usage: " << argv[0] << " xmap_filename\n";
        return 1;
    }
    const char* const xmap_filename = argv[1];

    TXRefMapped2DArray xmap(xmap_filename, true);
    for (unsigned row = 0; row < xmap.Size(); row++) {
        for (unsigned col = 0; col < xmap.GetLength(row); ++col) {
            const TXMapInfo info = xmap.GetAt(row, col);
            Cout << row << '\t' << col;

            if (info.EntryType == TXMapInfo::ET_LINK) {
                const TXMapLinkInfo& li = info.LinkInfo;
                Cout << '\t' << 'l'
                     << '\t' << li.OwnerFromId
                     << '\t' << li.Date
                     << '\t' << li.IsInternal
                     << '\t' << li.OrderNumberWithThisText
                     << '\t' << li.OwnerFromTIC
                     << '\t' << li.OwnersMatchedCategoryCount
                     << '\t' << li.HasGoodlink
                     << '\t' << li.OwnerGeo
                     << '\t' << li.SegmentType
                     << '\t' << li.NewCommercialWeight
                     << '\t' << li.NewCommercialLength
                     << '\t' << li.SrcLang
                     << '\t' << li.BanFlags
                     << '\t' << li.IsPorno;

            } else if (info.EntryType == TXMapInfo::ET_CATALOG) {
                Cout << '\t' << 'c';

            }

            const TXMapTextInfo& ti = info.TextInfo;
            Cout << '\t' << ti.GetWordsIdfSum()
                 << '\t' << ti.GetTwoWordsIdfSum()
                 << '\t' << ti.GetPornoWordsIdfSum()
                 << '\t' << ti.WordCount;

            Cout << '\n';
        }
    }
    return 0;
}
