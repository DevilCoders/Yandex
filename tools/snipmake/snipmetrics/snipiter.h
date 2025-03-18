#pragma once

#include <tools/snipmake/common/common.h>

#include <kernel/qtree/richrequest/richnode.h>

#include <util/generic/string.h>
#include <util/stream/input.h>
#include <util/stream/mem.h>
#include <util/string/strip.h>
#include <library/cpp/string_utils/quote/quote.h>
#include <util/string/vector.h>
#include <util/string/split.h>

namespace NSnippets {

    class TSnipRawIter : public ISnippetsIterator {
    public:
        static const char DELIM_T[];

        TReqSnip Res;
        IInputStream* Inp;

        TSnipRawIter(IInputStream* inp)
          : Inp(inp)
        {
        }

        const TReqSnip& Get() const override {
            return Res;
        }

        bool Next() override {
            TString line;
            if (!Inp->ReadLine(line) || (line.length() == 0)) {
                return false;
            }

            size_t pos = 0;
            while ( (pos = line.find("\t\t")) != TString::npos) {
                line.replace(pos, 2, "\t \t");
            }

            TVector<TString> fields;
            StringSplitter(line).SplitByString(DELIM_T).SkipEmpty().Collect(&fields);

            const size_t fsize = fields.size();

            if (fsize < 7) {
                return false;
            }

            Res.Id = fields[0];
            Res.Query = fields[1];
            TString qtree = fields[2];
            CGIUnescape(qtree);
            Res.RichRequestTree = DeserializeRichTree(DecodeRichTreeBase64(qtree)); // Build reach tree.
            Res.B64QTree = qtree;
            Res.Url = UTF8ToWide(fields[3]);
            Res.DocumentPath = fields[4];
            Res.TitleText = UTF8ToWide(fields[5]);
            Res.SnipText.clear();

            if (fsize > 6 && StripInPlace(fields[6]).empty() == false)
                Res.SnipText.push_back(UTF8ToWide(fields[6]));

            Res.ExtraInfo.clear();
            if (fsize > 7) {
                if (fields[7] != " ") {
                    //headline, shouldn't really happen
                    Res.SnipText.insert(Res.SnipText.begin(), UTF8ToWide(fields[7]));
                }
                if (fsize > 8) {
                    Res.ExtraInfo = fields[8];
                }
            }
            return true;
       }

        ~TSnipRawIter() override {}
    };
}
