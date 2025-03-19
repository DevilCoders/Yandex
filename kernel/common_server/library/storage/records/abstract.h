#pragma once
#include <util/system/types.h>
#include <util/generic/string.h>
#include "db_value.h"

namespace NCS {
    namespace NStorage {
        enum class EDataCodec {
            Null = 0 /* "null" */,
            FastLZ = 1 /* "fastlz" */,
            LZ4 /* "lz4" */,
            Snappy /* "snappy" */,
            BZip2 /* "bzip2" */,
            ZLib /* "zlib" */
        };

        class TTableRecordWT;

        class IBaseRecordsSet {
        protected:
            ui32 ErrorsCount = 0;
        public:
            virtual ~IBaseRecordsSet() = default;

            template <class T>
            T* GetAs() {
                return dynamic_cast<T*>(this);
            }

            virtual ui32 GetErrorsCount() const {
                return ErrorsCount;
            }
            void AddError() {
                ++ErrorsCount;
            }
        };

        class IRecordsSetWT: public IBaseRecordsSet {
        public:
            virtual bool AddRow(TTableRecordWT&& row) = 0;
            virtual void Reserve(const ui32 count) = 0;
        };

    }
}
