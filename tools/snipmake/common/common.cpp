#include "common.h"

#include <util/string/vector.h>
#include <util/string/split.h>
#include <util/string/strip.h>

namespace NSnippets {

    void TReqSnip::MergeFragments() {
        TSnipFragment res;
        for (size_t i = 0; i < SnipText.size(); ++i) {
            if (res.Text.size()) {
                res.Text += u" ... ";
            }
            res.Text += SnipText[i].Text;
            if (res.Coords.size()) {
                res.Coords += " ";
            }
            res.Coords += SnipText[i].Coords;
            if (res.ArcCoords.size()) {
                res.ArcCoords += " ";
            }
            res.ArcCoords += SnipText[i].ArcCoords;
        }
        SnipText.clear();
        SnipText.push_back(res);
    }

    void ParseTsdIntoSnippet(const TString& tsd, TReqSnip* snippet)
    {
        TVector<TStringBuf> fields;
        {
            TCharDelimiter<const char> d('\t');
            TContainerConsumer< TVector<TStringBuf> > c(&fields);
            SplitString(tsd.data(), tsd.data() + tsd.size(), d, c);
        }
        if (fields.size() < 14) {
            return;
        }
        snippet->Id.AssignNoAlias(fields[0]);
        snippet->Query.AssignNoAlias(fields[1]);
        snippet->Region.AssignNoAlias(fields[2]);
        snippet->Url = UTF8ToWide(fields[3].data(), fields[3].size());
        snippet->Relevance.AssignNoAlias(fields[4]);
        snippet->TitleText = UTF8ToWide(fields[5].data(), fields[5].size());
        TSnipFragment s; //comes in one piece
        s.Text = UTF8ToWide(fields[6].data(), fields[6].size()); //not splitting text
        snippet->Algo.AssignNoAlias(fields[7]);
        snippet->Rank.AssignNoAlias(fields[8]);
        s.Coords.AssignNoAlias(fields[9]); //Oww: not splitting coords, though they are splittable by ' '
        snippet->SnipText.push_back(s);
        snippet->FeatureString.AssignNoAlias(fields[10]);
        TSnipMark m;
        TStringBuf cv = fields[11];
        size_t d = cv.find(':');
        if (d == TStringBuf::npos) {
            m.Criteria.clear();
            m.Value = 0;
        } else {
            m.Criteria = UTF8ToWide(cv.data(), d - 1);
            m.Value = FromString<int>(cv.data() + d + 1, cv.size() - 1 - d);
        }
        m.Quality = 0.0f; //absent in tsd
        m.Assessor = UTF8ToWide(fields[12].data(), fields[12].size());
        m.Timestamp = UTF8ToWide(fields[13].data(), fields[13].size());
        snippet->Marks.push_back(m);
    }

    void ParseDumpsnipcandsCandIntoTSnippet(const TString& dumpsnipcandsCand, TReqSnip* snippet)
    {
        TVector<TString> fields;
        StringSplitter(dumpsnipcandsCand).Split('\t').Collect(&fields);

        int fragCnt = FromString<int>(fields[0]);
        int shift = fragCnt - 1;

        TString coordsStroka;
        Strip(fields[shift + 4], coordsStroka);

        TVector<TString> coords;
        StringSplitter(coordsStroka).Split(' ').SkipEmpty().Collect(&coords);

        for (int i = 0; i < fragCnt; ++i) {
            TSnipFragment fragment;
            fragment.Text = UTF8ToWide(fields[i + 1]);
            fragment.Coords = coords[i];
            snippet->SnipText.push_back(fragment);
        }

        snippet->Algo = fields[shift + 2];
        snippet->Rank = fields[shift + 3];
        snippet->FeatureString = fields[shift + 5];
        if (fields.ysize() > shift + 6 && !!fields[shift+6]) {
            TVector<TString> arcCoords;
            StringSplitter(fields[shift + 6]).Split(' ').SkipEmpty().Collect(&arcCoords);
            for (int i = 0; i < fragCnt; ++i) {
                snippet->SnipText[i].ArcCoords = arcCoords[i];
            }
        }

        TString title;
        if (fields.ysize() > shift + 7) {
            title = fields[shift + 7];
            StripInPlace(title);
        }
        if (!!title) {
            snippet->TitleText = UTF8ToWide(title);
        }
        if (fields.ysize() > shift + 8) {
            snippet->ImgUH = fields[shift + 8];
        }
    }

}
