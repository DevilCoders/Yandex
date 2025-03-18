#include <kernel/lemmer/core/language.h>
#include <library/cpp/langs/langs.h>
#include <util/memory/blob.h>
#include <util/generic/string.h>
#include <util/stream/file.h>
#include <util/stream/input.h>
#include <util/stream/output.h>
#include <ysite/yandex/pure/pure_header.h>

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        Cerr << "This tool writes actual lemmer fingerprints to pure files\n";
        Cerr << "Usage: ./pure_fingerprints old_pure_file [new_pure_file]\n";
        return 1;
    }
    TString in = argv[1];
    TString out = argc == 3 ? argv[2] : in;
    TString fp_file = in + ".fingerprints";

    TBlob inBlob = TBlob::FromFileContent(in.data());
    NPure::TPureHeaderReader inHeader(inBlob);
    TBlob subBlob = inBlob.SubBlob(static_cast<size_t>(inHeader.GetTrieOffset()), inBlob.Length());

    {
        TFixedBufferFileOutput outFp(fp_file.data());
        for (const auto& fp : inHeader.GetLangMap()) {
            const TLanguage * lang = NLemmer::GetLanguageById(fp.first);
            const auto& code = lang->Code();
            const auto& old_fp = fp.second;
            const auto& new_fp = lang->DictionaryFingerprint();
            if (old_fp != new_fp)
                Cerr << code << ": " << old_fp << " -> " << new_fp << Endl;
            outFp << "    " << code << ": " << new_fp << Endl;
        }
    }

    NPure::TPureHeaderWriter outHeader(fp_file.data());

    TFixedBufferFileOutput outFile(out.data());
    outHeader.WriteHeader(outFile);
    outFile.Write(subBlob.AsCharPtr(), subBlob.Length());
}
