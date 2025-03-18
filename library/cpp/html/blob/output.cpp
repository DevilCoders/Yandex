#include "output.h"

#include <library/cpp/packedtypes/longs.h>

#include <util/generic/algorithm.h>

namespace NHtml {
    namespace NBlob {
        TOutput::TOutput(IOutputStream* slave)
            : Slave_(slave)
            , Pos_(0)
        {
        }

        TOutput::~TOutput() {
        }

        ui32 TOutput::Position() const {
            return Pos_;
        }

        void TOutput::WriteUInt8(ui8 value) {
            Slave_->Write(&value, sizeof(value));
            Pos_ += sizeof(value);
        }

        void TOutput::WriteVariableInt32(i32 value) {
            char buf[5];
            int len = out_long(value, buf);

            Slave_->Write(buf, len);
            Pos_ += len;
        }

        void TOutput::WriteBytes(const void* data, int len) {
            Slave_->Write(data, len);
            Pos_ += len;
        }

        void TOutput::WriteString(const TString& value) {
            WriteVariableInt32(value.size());
            WriteBytes(value.data(), value.size());
        }

        void TDictionaries::Save(TOutput* output, TDocumentPack::TStrings* index) const {
            if (TagIds_) {
                index->MutableTags()->CopyFrom(WriteStrings(TagIds_, output));
            }
            if (AttrNames_) {
                index->MutableAttributeNames()->CopyFrom(WriteStrings(AttrNames_, output));
            }
            if (AttrValues_) {
                index->MutableAttributeValues()->CopyFrom(WriteStrings(AttrValues_, output));
            }
            if (Texts_) {
                index->MutableTexts()->CopyFrom(WriteStrings(Texts_, output));
            }
            if (StyleNames_) {
                index->MutableStyleNames()->CopyFrom(WriteStrings(StyleNames_, output));
            }
            if (StyleValues_) {
                index->MutableStyleValues()->CopyFrom(WriteStrings(StyleValues_, output));
            }
        }

        i32 TDictionaries::GetStringId(const TString& name, TStringIds* ids) {
            auto mi = ids->find(name);
            if (mi == ids->end()) {
                ids->insert(std::make_pair(name, ids->size() + 1));
                return ids->size();
            } else {
                return mi->second;
            }
        }

        TRange TDictionaries::WriteStrings(const TStringIds& ids, TOutput* output) const {
            TRange range;
            TStringVector strings(ids.begin(), ids.end());
            // Сортируем строки так, чтобы позиция каждой строки в векторе
            // соответствовала бы её индексу.
            Sort(strings.begin(), strings.end(), [](const auto& x, const auto& y) {
                return x.second < y.second;
            });
            range.SetBegin(output->Position());
            output->WriteVariableInt32((i32)strings.size());
            for (auto mi = strings.begin(); mi != strings.end(); ++mi) {
                output->WriteString(mi->first);
            }
            range.SetEnd(output->Position());
            return range;
        }

    }
}
