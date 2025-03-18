#include <nginx/modules/strm_packager/src/common/sender.h>

#include <nginx/modules/strm_packager/src/common/evp_cipher.h>

namespace NStrm::NPackager {
    TSender::TSender(TRequestWorker& request)
        : Request(request)
        , IsDataSet(false)
    {
    }

    void TSender::SetData(const TRepFuture<TSendData>& data) {
        Y_ENSURE(!IsDataSet);
        IsDataSet = true;
        data.AddCallback(Request.MakeFatalOnException(
            [this](const TRepFuture<TSendData>::TDataRef& dr) {
                if (dr.Empty()) {
                    Finish();
                } else {
                    Send(dr.Data());
                }
            }));
    }

    void TSender::Finish() {
        Request.Finish();
    }

    void TSender::Send(const TSendData& data) {
        Request.SendData(data);
    }

    // sender, that encrypt data with AES-128 CBC with padding
    class TCipheredSender: public TSender {
    public:
        TCipheredSender(TRequestWorker& request, const TDrmInfo& drmInfo);

    private:
        void Send(const TSendData& data) override;
        void Finish() override;

    private:
        TEvpCipher Cipher;
    };

    TCipheredSender::TCipheredSender(TRequestWorker& request, const TDrmInfo& drmInfo)
        : TSender(request)
        , Cipher(EVP_aes_128_cbc())
    {
        Y_ENSURE(drmInfo.IV.Defined());
        Y_ENSURE(drmInfo.IV->length() == Cipher.IVLength());
        Y_ENSURE(drmInfo.Key.length() == Cipher.KeyLength());
        Y_ENSURE(Cipher.BlockSize() == 16);
        Cipher.EncryptInit((ui8 const*)drmInfo.Key.Data(), (ui8 const*)drmInfo.IV->Data(), /* padding = */ true);
    }

    void TCipheredSender::Finish() {
        const size_t predictedFinalSize = Cipher.PredictEncryptFinal();

        TSendData data;
        if (predictedFinalSize > 0) {
            data.Blob = TSimpleBlob(Request.MemoryPool.AllocateArray<ui8>(predictedFinalSize, /*align = */ 1), predictedFinalSize);
        }

        ui8 dummy[Cipher.BlockSize()];
        const size_t actualFinalSize = Cipher.EncryptFinal(
            /* output    = */ predictedFinalSize > 0 ? (ui8*)data.Blob.data() : dummy);

        Y_ENSURE(actualFinalSize == predictedFinalSize);

        if (!data.Blob.empty()) {
            Request.SendData(data);
        }

        Request.Finish();
    }

    void TCipheredSender::Send(const TSendData& dataIn) {
        TSendData data;
        data.Flush = dataIn.Flush;

        if (dataIn.ContentLengthPromise.Defined()) {
            data.ContentLengthPromise = Cipher.PredictEncryptedSize(*dataIn.ContentLengthPromise);
        }

        if (dataIn.Blob.empty()) {
            Request.SendData(data);
            return;
        }

        const size_t predictedUpdateSize = Cipher.PredictEncryptUpdate(dataIn.Blob.size());

        if (predictedUpdateSize > 0) {
            data.Blob = TSimpleBlob(Request.MemoryPool.AllocateArray<ui8>(predictedUpdateSize, /*align = */ 1), predictedUpdateSize);
        }

        ui8 dummy[Cipher.BlockSize()];
        const size_t actualUpdateSize = Cipher.EncryptUpdate(
            /* output    = */ predictedUpdateSize > 0 ? (ui8*)data.Blob.data() : dummy,
            /* input     = */ dataIn.Blob.data(),
            /* inputSize = */ dataIn.Blob.size());

        Y_ENSURE(actualUpdateSize == predictedUpdateSize);

        Request.SendData(data);
    }

    // static
    TSenderFuture TSender::Make(TRequestWorker& request) {
        TSender* sender = request.GetPoolUtil<TSender>().New(request);
        return NThreading::MakeFuture<ISender*>(sender);
    }

    // static
    TSenderFuture TSender::Make(TRequestWorker& request, const NThreading::TFuture<TMaybe<TDrmInfo>>& drmFuture) {
        return drmFuture.Apply([&request](const NThreading::TFuture<TMaybe<TDrmInfo>>& future) -> ISender* {
            const TMaybe<TDrmInfo>& drmInfo = future.GetValue();

            if (drmInfo.Empty()) {
                return request.GetPoolUtil<TSender>().New(request);
            }

            return request.GetPoolUtil<TCipheredSender>().New(request, *drmInfo);
        });
    }

}
