#pragma once

#include <library/cpp/codecs/tls_cache.h>
#include <library/cpp/on_disk/coded_blob/common/coded_blob_utils.h>

#include <util/memory/blob.h>
#include <util/system/tls.h>

namespace NCodedBlob {
    class TBasicBlob {
    protected:
        struct TRecordLayout {
            ui64 HeaderLength = 0;
            ui64 DataLength = 0;

            TRecordLayout() = default;

            TRecordLayout(ui64 h, ui64 v)
                : HeaderLength(h)
                , DataLength(v)
            {
            }

            ui64 RecordLength() const {
                return HeaderLength + DataLength;
            }
        };

        class TRawOffsetIterator {
        public:
            ui64 GetCurrentOffset() const {
                return CurrentOffset;
            }

            TRecordLayout GetCurrentLayout() const {
                return CurrentLayout;
            }

            TStringBuf GetCurrentRawData(const TBasicBlob& blob) const {
                return blob.DoGetRawData(CurrentOffset);
            }

            bool HasNext(const TBasicBlob& blob) const {
                return CurrentOffset + CurrentLayout.RecordLength() < blob.Data.Size();
            }

            void Next(const TBasicBlob& blob) {
                CurrentOffset += CurrentLayout.RecordLength();
                CurrentLayout = blob.DoGetRawRecordLayout(blob.DoGetRawRecord(CurrentOffset));
            }

        private:
            ui64 CurrentOffset = NUtils::INVALID_OFFSET;
            TRecordLayout CurrentLayout{1, 0};
        };

    protected:
        virtual ~TBasicBlob() {
        }

        virtual void InitHeader(TBlob&) = 0;

        virtual void InitFooter(TBlob&) = 0;

        Y_FORCE_INLINE TStringBuf DoGetRawRecord(ui64 offset) const {
            return TStringBuf(Data.AsCharPtr() + offset, Data.Size() - offset);
        }

        Y_FORCE_INLINE TRecordLayout DoGetRawRecordLayout(TStringBuf record) const {
            NUtils::TLengthPacker packer;
            TRecordLayout lout;
            packer.UnpackLeaf(record.data(), lout.DataLength);
            lout.HeaderLength = packer.SkipLeaf(record.data());

            return lout;
        }

        Y_FORCE_INLINE TStringBuf DoGetRawData(ui64 offset) const {
            TStringBuf record = DoGetRawRecord(offset);
            TRecordLayout lout = DoGetRawRecordLayout(record);

            return record.SubStr(lout.HeaderLength, lout.DataLength);
        }

    public:
        void Init(TBlob b);

        ui64 Size() const {
            return TotalSize;
        }

        ui64 Count() const {
            return EntryCount;
        }

    protected:
        TBlob Data;
        ui64 EntryCount = 0;
        ui64 TotalSize = 0;
    };

    const ui64 CODED_BLOB_VERSION = 0;
    const char CODED_BLOB_MAGIC[] = "CODEDBLB";

    class TCodedBlob: public TBasicBlob {
    public:
        class TOffsetIterator {
        public:
            TOffsetIterator(const TCodedBlob* blob = nullptr)
                : Blob(blob)
            {
            }

            ui64 GetCurrentOffset() const {
                return RawIterator.GetCurrentOffset();
            }

            TStringBuf GetCurrentValue() const {
                return ValueHelper.GetCurrent(this);
            }

            bool HasNext() const {
                if (!Blob) {
                    return false;
                }

                return RawIterator.HasNext(*Blob);
            }

            bool Next() {
                if (!HasNext()) {
                    return false;
                }

                RawIterator.Next(*Blob);
                ValueHelper.Invalidate();

                return true;
            }

        private:
            TStringBuf FetchCurrent(NUtils::TValueTag, TBuffer& b) const {
                return Blob->GetByOffset(RawIterator.GetCurrentOffset(), b);
            }

        private:
            NUtils::TValueHelper ValueHelper;

            TRawOffsetIterator RawIterator;
            const TCodedBlob* Blob = nullptr;

            friend NUtils::TValueHelper;
        };

    public:
        TCodedBlob() = default;

        TCodedBlob(const TBlob& b) {
            Init(b);
        }

        void InitHeader(TBlob& b) override;

        void InitFooter(TBlob& b) override;

        TString GetCodecName() const {
            return NCodecs::ICodec::GetNameSafe(Codec);
        }

        TOffsetIterator OffsetIterator() const {
            return TOffsetIterator(this);
        }

        TStringBuf GetByOffset(ui64 offset) const {
            auto tmpBuffer = NCodecs::TBufferTlsCache::TlsInstance().Item();
            return GetByOffset(offset, tmpBuffer.Get());
        }

        TStringBuf GetByOffset(ui64 offset, TBuffer& buffer) const {
            TStringBuf res = DoGetRawData(offset);

            if (!Codec) {
                return res;
            }

            return Decode(res, buffer);
        }

        TStringBuf GetByOffsetRaw(ui64 offset) const {
            return DoGetRawData(offset);
        }

        TString GetSerializedCodecData() const {
            TString result;
            TStringOutput str(result);
            NCodecs::ICodec::Store(&str, Codec);

            return result;
        }
    private:
        TStringBuf Decode(TStringBuf data, TBuffer& buffer) const {
            buffer.Clear();
            Codec->Decode(data, buffer);

            return TStringBuf(buffer.data(), buffer.size());
        }

    private:
        NCodecs::TCodecPtr Codec;
    };

}
