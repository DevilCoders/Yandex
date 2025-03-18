#include "internal.h"

#include <library/cpp/packedtypes/longs.h>
#include <google/protobuf/io/coded_stream.h>

namespace NHtml {
    namespace NBlob {
        using namespace google::protobuf;

        bool GetMetadata(const TStringBuf& data, TDocumentPack* meta) {
            if (data.size() > 4) {
                const char* p = data.data() + data.size() - 4;
                ui32 size;

                memcpy(&size, p, sizeof(size));
                // Перемещаем указатель на начало таблицы индекса.
                p -= size;
                // Проверка корректности размера таблицы индекса.
                if (p >= data.data() && p < data.data() + data.size() - 4) {
                    io::CodedInputStream input((const ui8*)p, size);

                    input.SetTotalBytesLimit(512 << 20);
                    input.SetRecursionLimit(1024);

                    return meta->ParseFromCodedStream(&input) &&
                           input.ConsumedEntireMessage();
                }
            }

            return false;
        }

        bool GetStrings(const TStringBuf& data, TVector<TString>* result) {
            const char* p = data.data();
            const char* end = data.data() + data.size();
            i32 count;
            i32 i = 0;

            p += in_long(count, p);
            // Необходимо зарезервировать один дополнительный элемент, так как
            // индексы всех строк начинаются с 1.
            result->reserve(count + 1);
            result->push_back(TString());

            for (; i < count && p < end; ++i) {
                i32 len;

                p += in_long(len, p);
                result->push_back(TString(p, len));
                p += len;
            }

            return p == end && i == count;
        }

    }
}
