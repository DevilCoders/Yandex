/// author@ cheusov@ Aleksey Cheusov
/// created: Wed,  8 Oct 2014 13:47:48 +0300
/// see: OXYGEN-895, OXYGEN-903

#include <cassert>

#include <library/cpp/getopt/opt2.h>

#include <kernel/sent_lens/sent_lens.h>


void DumpAll(const TString& indexPath) {
    TSentenceLengthsReader reader(indexPath);

    Cout << "Version: " << reader.GetVersion() << '\n';
    size_t size = reader.GetSize();
    Cout << "Size: " << size << '\n';

    TSentenceLengths lengths;
    TSentenceOffsets offsets;

    for (size_t i = 0; i < size; ++i) {
        reader.Get(i, &lengths);
        reader.GetOffsets(i, &offsets);

        size_t lenSize = lengths.size();
        Y_ASSERT(lenSize + 1 == offsets.size());

        for (size_t j = 0; j < lenSize; ++j) // FIXME this way we do not print the last offset
            Cout << i << ' ' << j << ": "
                 << (ui64) offsets[j] << ' ' << (ui64) lengths [j] << '\n';
    }
}


void DumpSelected(const TString& indexPath, size_t firstDoc, size_t lastDoc) {
    TVector<size_t> selected;
    selected.push_back(firstDoc);
    Y_ENSURE(lastDoc == (size_t)-1 || lastDoc >= firstDoc, "Invalid docs range");
    while (lastDoc != (size_t)-1 && selected.back() != lastDoc) {
        selected.push_back(selected.back() + 1);
    }

    TSentenceLengthsReader reader(indexPath);

    Cout << "Version: " << reader.GetVersion() << '\n';
    size_t size = reader.GetSize();
    Cout << "Num Docs: " << size << '\n';

    TSentenceLengths lengths;
    TSentenceOffsets offsets;
    for (size_t docid : selected) {
        reader.Get(docid, &lengths);
        reader.GetOffsets(docid, &offsets);

        size_t lenSize = lengths.size();
        Y_ASSERT(lenSize + 1 == offsets.size());

        Cout << "docid: " << docid << "\t";
        for (auto ln : lengths) {
            Cout << (size_t)ln << " ";
        }
        Cout << Endl;

        Cout << "docid: " << docid << "\t";
        for (auto ofs : offsets) {
            Cout << (size_t)ofs << " ";
        }
        Cout << Endl;
    }
}


int main(int argc, char **argv)
{
    Opt2 opts(argc, const_cast<char* const *>(argv), "f:l:", 1);
    const TString fdi_str = opts.Arg('f', "- first doc id. (Dump all if absent)", "");
    const TString ldi_str = opts.Arg('l', "- last doc id. (Dump only first if absent)", "");
    const size_t firstDocId = fdi_str.size() ? FromString<size_t>(fdi_str) : -1;
    const size_t lastDocId = ldi_str.size() ? FromString<size_t>(ldi_str) : -1;
    opts.AutoUsageErr("index_path");
    TString indexPath = opts.Pos[0];
    if (firstDocId == (size_t)-1) {
        DumpAll(indexPath);
    } else {
        DumpSelected(indexPath, firstDocId, lastDocId);
    }
    return 0;
}
