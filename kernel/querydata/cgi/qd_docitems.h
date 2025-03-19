#pragma once

#include <kernel/querydata/common/querydata_traits.h>

#include <kernel/hosts/owner/owner.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>
#include <util/string/vector.h>

#include <utility>

namespace NQueryData {

    enum EDocItemMode {
        DIM_SNIPS = 1,
        DIM_BANS = 2,
        DIM_ALL = DIM_SNIPS | DIM_BANS
    };

    class TDocItemsRec {
        TMemoryPool Pool{1024};
        TStringBufs DocIds_;
        TStringBufs SnipDocIds_;
        TStringBufs Categs_;
        TStringBufs SnipCategs_;
        TStringBufPairs Urls_;
        TStringBufPairs SnipUrls_;

        bool OwnMemory = true;

    public:
        explicit operator bool() const {
            return DocIds_ || SnipDocIds_ || Categs_ || SnipCategs_ || Urls_ || SnipUrls_;
        }

        void SetOwnMemory(bool own) {
            OwnMemory = own;
        }

        const TStringBufs& DocIds() const {
            return DocIds_;
        }

        const TStringBufs& SnipDocIds() const {
            return SnipDocIds_;
        }

        const TStringBufs& Categs() const {
            return Categs_;
        }

        const TStringBufs& SnipCategs() const {
            return SnipCategs_;
        }

        const TStringBufPairs& Urls() const {
            return Urls_;
        }

        const TStringBufPairs& SnipUrls() const {
            return SnipUrls_;
        }

        TStringBufs AllDocIdsNoCopy(TMemoryPool&, EDocItemMode = DIM_ALL) const;

        TStringBufs AllCategUrlsNoCopy(TMemoryPool&, EDocItemMode = DIM_ALL) const;

        TStringBufs AllCategsNoCopy(EDocItemMode mode = DIM_ALL) const;

        TStringBufs AllUrlsNoCopy(EDocItemMode mode = DIM_ALL) const;

        TStringBufs& MutableCategs() {
            return Categs_;
        }

        TStringBufPairs& MutableUrls() {
            return Urls_;
        }

        void FromJson(const NSc::TValue&);
        void ToJson(NSc::TValue&) const;
        NSc::TValue ToJson() const;

        void AddDocId(TStringBuf docId) {
            AddDocIdAny(docId, DIM_BANS);
        }

        void AddSnipDocId(TStringBuf docId) {
            AddDocIdAny(docId, DIM_SNIPS);
        }

        void AddCateg(TStringBuf categ) {
            AddCategAny(categ, DIM_BANS);
        }

        void AddSnipCateg(TStringBuf categ) {
            AddCategAny(categ, DIM_SNIPS);
        }

        void AddUrl(TStringBuf url, TStringBuf categ) {
            AddUrlAny(url, categ, DIM_BANS);
        }

        void AddSnipUrl(TStringBuf url, TStringBuf categ) {
            AddUrlAny(url, categ, DIM_SNIPS);
        }

        void AddDocIdAny(TStringBuf docId, EDocItemMode dm) {
            DoAdd(dm & DIM_SNIPS ? SnipDocIds_ : DocIds_, docId);
        }

        void AddCategAny(TStringBuf categ, EDocItemMode dm) {
            DoAdd(dm & DIM_SNIPS ? SnipCategs_ : Categs_, categ);
        }

        void AddUrlAny(TStringBuf url, TStringBuf categ, EDocItemMode dm) {
            DoAddUrlRaw(dm & DIM_SNIPS ? SnipUrls_ : Urls_, url, categ);
        }

        template <typename T>
        void AddDocIdsAny(const T& t, EDocItemMode dm) {
            DoAddColl(dm & DIM_SNIPS ? SnipDocIds_ : DocIds_, t);
        }

        template <typename T>
        void AddCategsAny(const T& t, EDocItemMode dm) {
            DoAddColl(dm & DIM_SNIPS ? SnipCategs_ : Categs_, t);
        }

        void AddUrlsAny(const NSc::TValue& data, EDocItemMode dm) {
            DoAddUrls(dm & DIM_SNIPS ? SnipUrls_ : Urls_, data);
        }

    private:
        void DoAddUrlWithNormalization(TStringBufPairs& urls, TStringBuf url);

        void DoAddUrlRaw(TStringBufPairs& urls, TStringBuf url, TStringBuf categ);

        void DoAddUrls(TStringBufPairs& urls, const NSc::TValue& data);

        TStringBuf Own(TStringBuf item) {
            return OwnMemory && item ? Pool.AppendString(item) : item;
        }

        void DoAdd(TStringBufs& vec, TStringBuf item) {
            if (item) {
                vec.emplace_back(Own(item));
            }
        }

        template <typename T, typename TColl>
        void DoAddColl(TVector<T>& vec, const TColl& item) {
            for (const auto& c : item) {
                DoAdd(vec, c);
            }
        }
    };


    TVector<TString> PairsToStrings(const TStringBufPairs& t);

    TString SplitDictStrings(const TVector<TString>& dictItems, ui32 part, ui32 parts);

}
