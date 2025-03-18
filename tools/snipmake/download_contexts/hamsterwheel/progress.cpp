#include "progress.h"
#include <util/string/cast.h>

namespace NSnippets
{
    TProgressSaver::TProgressSaver(const TString& fileName)
        : ProgressFile(fileName, OpenAlways | RdWr | Seq | ForAppend)
        , Out(ProgressFile)
    {
        TString s;
        TFileInput inf(ProgressFile);
        while (inf.ReadLine(s)) {
            try {
                size_t val = FromString(s);
                WorkDone.insert(val);
            } catch (TFromStringException&) {
                // pass
            }
        }
        ProgressFile.Seek(0, sEnd);
    }

    void TProgressSaver::MarkCompleted(size_t tag)
    {
        Out << tag << Endl;
        WorkDone.insert(tag);
    }

    bool TProgressSaver::IsCompleted(size_t tag)
    {
        return WorkDone.contains(tag);
    }
} // namespace NSnippets
