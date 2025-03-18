#pragma once

#include <util/generic/strbuf.h>

#include "geomapping.h"

namespace NReverseGeocoder {
    namespace NYandexMap {
        struct ToponymTraits;

        class ToponymChecker {
        public:
            ToponymChecker();
            virtual ~ToponymChecker();

            void ShowReminder() const;
            bool IsToponymOK(ToponymTraits& traits);

        protected:
            void InsertItem(const TMappingTraits& traits);

            virtual bool IsToponymOkImpl(ToponymTraits& traits);

            virtual bool SearchId(int id);
            virtual bool SearchName(TStringBuf name) = 0;

            void RemoveCurIt();

            virtual bool IsKindOk(const TString& kind) const;
            virtual bool IsNameOk(const TString& name) const;
            const TMappingTraits& GetMappingTraits() const;

            Id2ToponymMappingType Mapping;
            Id2ToponymMappingType::const_iterator MapIt;
        };

        class CheckerViaGeomapping: public ToponymChecker {
        public:
            CheckerViaGeomapping(TStringBuf geoMappingFileName);

        protected:
            int FindMatchedRegionId(const ToponymTraits& traits) const;
            bool IsToponymOkImpl(ToponymTraits& traits) override;
            bool SearchName(TStringBuf name) override;

        private:
            TTopId2RegIdMapping SourceId2RegIdMap;
            TTopId2RegIdMapping ToponymId2RegIdMap;
        };

        class CheckerViaGeodata: public ToponymChecker {
        public:
            CheckerViaGeodata(TStringBuf geoDataFileName, int countryId);

        protected:
            bool IsNameOk(const TString& name) const override;
            bool SearchName(TStringBuf name) override;
        };
    }
}
