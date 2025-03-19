#pragma once

#include "erfattrs.h"
#include <kernel/indexer/faceproc/docattrs.h>
#include "kernel/news_annotations/news_orange_annotations.h"

#include <kernel/tarc/iface/tarcface.h> // for ARCHIVE_FIELD_VALUE_LIST_SEP

#include <yweb/protos/indexeddoc.pb.h>
#include <yweb/protos/navsource.pb.h>
#include <yweb/protos/robot.pb.h>

#include <util/generic/hash_set.h> // for THashSet

class SDocErfInfo3;
class TFullDocAttrs;
namespace NRealTime {
    class TIndexedDoc;
};

class TAnnotationProcessor {
public:
    typedef google::protobuf::RepeatedPtrField<NOrangeData::TDocAnnotation> TDocAnnotations;
    typedef TDocAnnotations::const_iterator TAnnotationIterator;
    typedef google::protobuf::RepeatedPtrField<TString> TValues;
    typedef TValues::const_iterator TValuesIterator;

    class IAnnotationParser {
    public:
        virtual bool ProcessAnnotationAttrs(const TString& /*source*/, const TString& /*type*/, const ui32 /*modTime*/) = 0;
        virtual void ProcessAnnotationValue(const TString& annotationValue) = 0;
        virtual ~IAnnotationParser() {
        };
    };

    class TSnippetsParser : public IAnnotationParser {
    private:
        ui32 LastSnippetModTime; // no mod time
        std::pair<TString, TString> StoredSnippet;
        bool HadPreferredSource;
        const TString PreferredSource;

        ui32 ModTime;
        size_t BadSnippetsNum;

    public:
        TSnippetsParser(const TString& preferredSource);

        void Init();
        bool ProcessAnnotationAttrs(const TString& source, const TString& /*type*/, const ui32 modTime) override;
        void ProcessAnnotationValue(const TString& annotationValue) override;
        void SetSnippet(TFullDocAttrs& docAttrs);
        const std::pair<TString, TString>& GetSnippet() const {
            return StoredSnippet;
        }

        size_t GetBadSnippetsNum() const {
            return BadSnippetsNum;
        }
    };

    class TUrlHashParser : public IAnnotationParser {
    private:
        THashSet<ui64> UniqueHashes;
        size_t BadUrlHashesNum;
        bool UseErfCreator;

    public:
        TUrlHashParser(bool useErfCreator);

        void Init();
        bool ProcessAnnotationAttrs(const TString& source, const TString& /*type*/, const ui32 modTime) override;
        void ProcessAnnotationValue(const TString& annotationValue) override;
        void SetUniqueHashes(const ui64 erfUrlHash, NRealTime::TIndexedDoc& indexedDoc) const;

        size_t GetBadUrlHashesNum() const {
            return BadUrlHashesNum;
        }
    };

    class TNavigationalParser : public IAnnotationParser {
    private:
        NRobot::TNavInfo NavInfo;
        size_t BadNavigationalsNum;

    public:

        void Init();
        bool ProcessAnnotationAttrs(const TString& source, const TString& /*type*/, const ui32 modTime) override;
        void ProcessAnnotationValue(const TString& annotationValue) override;
        void SetNavigationalInfo(NRobot::TNavInfo& navInfo);

        size_t GetBadNavigationalsNum() const {
            return BadNavigationalsNum;
        }
    };

    class TNewsParser : public IAnnotationParser {
    private:
        ui32 LastModTime;
        ui32 CurrentModTime;
        TNewsOrangeAnnotation NewsOrangeAnnotations;

    public:
        void Init();
        bool ProcessAnnotationAttrs(const TString& source, const TString& /*type*/, const ui32 modTime) override;
        void ProcessAnnotationValue(const TString& annotationValue) override;
        bool UpdateLinks(TAnchorText* anchorText);
    };
private:
    TSnippetsParser SnippetsParser;
    TUrlHashParser UrlHashParser;
    TNavigationalParser NavigationalParser;
    TNewsParser NewsParser;

public:
    TAnnotationProcessor(const TString& prefferedSource, bool useErfCreator);

    void Process(
        const TDocAnnotations& annotations,
        bool useNewsAnnotationParser,
        TFullDocAttrs& docAttrs,
        NRealTime::TIndexedDoc& indexedDoc,
        TAnchorText* anchorText
        );

    void SetUniqueHashes(const ui64 erfUrlHash, NRealTime::TIndexedDoc& indexedDoc) const;

    size_t GetBadSnippetsNum() const {
        return SnippetsParser.GetBadSnippetsNum();
    }

    size_t GetBadUrlHashesNum() const {
        return UrlHashParser.GetBadUrlHashesNum();
    }

    size_t GetBadNavigationalsNum() const {
        return NavigationalParser.GetBadNavigationalsNum();
    }
};

ui32 TimestampFromFeed(const char* feed);

void FillErfTimeCrawlRankVisitRate(const TErfAttrs& attrs, SDocErfInfo3& erf);

typedef google::protobuf::RepeatedPtrField<NOrangeData::TAttribute> TAttributes;

template<class TAttrsStorage>
void AddOrangeAttrFromSearchAttrs(const TAttributes& attrs, TAttrsStorage* docAttrs) {

    for (TAttributes::const_iterator attr = attrs.begin(); attr != attrs.end(); ++attr) {
        TString res;
        const TString& attrValue = attr->GetValue();
        for (size_t j = 0; j < attrValue.size(); ++j) {
            if (attrValue[j] == ';')
                res.append(ARCHIVE_FIELD_VALUE_LIST_SEP);
            else
                res.append(attrValue[j]);
        }
        docAttrs->AddAttr(attr->GetName(), res, TFullDocAttrs::AttrArcText);
    }
}

template<class TAttrsStorage>
void AddOrangeAttrFromSearchAttrs(const TAnchorText& anchorText, TAttrsStorage* docAttrs) {
    const TAttributes& attrs = anchorText.GetSearchAttribute();
    AddOrangeAttrFromSearchAttrs(attrs, docAttrs);
}


