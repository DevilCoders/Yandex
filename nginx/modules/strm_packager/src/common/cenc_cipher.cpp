#include <nginx/modules/strm_packager/src/common/cenc_cipher.h>

#include <util/generic/yexception.h>

namespace NStrm::NPackager {
    TCencCipher::TCencCipher(EVP_CIPHER const* type)
        : EvpCipher(type)
    {
    }

    TCencCipherAes128Ctr::TCencCipherAes128Ctr()
        : TCencCipher(EVP_aes_128_ctr())
    {
        Y_ENSURE(EvpCipher.KeyLength() == 16);
        Y_ENSURE(EvpCipher.IVLength() == 16);
        Y_ENSURE(EvpCipher.BlockSize() == 1);
    }

    void TCencCipherAes128Ctr::EncryptInit(ui8 const* newkey, ui8 const* newiv) {
        std::memcpy(Key, newkey, 16);

        IV = (ui64(newiv[0]) << 56) |
             (ui64(newiv[1]) << 48) |
             (ui64(newiv[2]) << 40) |
             (ui64(newiv[3]) << 32) |
             (ui64(newiv[4]) << 24) |
             (ui64(newiv[5]) << 16) |
             (ui64(newiv[6]) << 8) |
             (ui64(newiv[7]) << 0);

        AtSampleStart = true;
    }

    size_t TCencCipherAes128Ctr::IVLength() const {
        return 8;
    }

    void TCencCipherAes128Ctr::GetIV(ui8* outiv) const {
        Y_ENSURE(AtSampleStart);
        outiv[0] = 0xff & (IV >> 56);
        outiv[1] = 0xff & (IV >> 48);
        outiv[2] = 0xff & (IV >> 40);
        outiv[3] = 0xff & (IV >> 32);
        outiv[4] = 0xff & (IV >> 24);
        outiv[5] = 0xff & (IV >> 16);
        outiv[6] = 0xff & (IV >> 8);
        outiv[7] = 0xff & (IV >> 0);
    }

    void TCencCipherAes128Ctr::NextSample() {
        ++IV;
        AtSampleStart = true;
    }

    void TCencCipherAes128Ctr::Encrypt(ui8* const output, ui8 const* const input, const size_t inputSize) {
        Y_ENSURE(inputSize > 0);

        if (AtSampleStart) {
            ui8 ivBytes[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
            GetIV(ivBytes);
            EvpCipher.EncryptInit(Key, ivBytes, /*padding = */ false);

            AtSampleStart = false;
        }

        Y_ENSURE(EvpCipher.EncryptUpdate(output, input, inputSize) == inputSize);
    }

    TCencCipherAes128Cbc::TCencCipherAes128Cbc()
        : TCencCipher(EVP_aes_128_cbc())
    {
        Y_ENSURE(EvpCipher.KeyLength() == 16);
        Y_ENSURE(EvpCipher.IVLength() == 16);
        Y_ENSURE(EvpCipher.BlockSize() == 16);
    }

    void TCencCipherAes128Cbc::EncryptInit(ui8 const* newkey, ui8 const* newiv) {
        EvpCipher.EncryptInit(newkey, newiv, /*padding = */ false);
        std::memcpy(IV, newiv, 16);
    }

    void TCencCipherAes128Cbc::GetIV(ui8* outiv) const {
        std::memcpy(outiv, IV, 16);
    }

    void TCencCipherAes128Cbc::NextSample() {
    }

    void TCencCipherAes128Cbc::Encrypt(ui8* output, ui8 const* input, const size_t inputSize) {
        Y_ENSURE(inputSize % 16 == 0);
        Y_ENSURE(inputSize > 0);

        Y_ENSURE(inputSize == EvpCipher.EncryptUpdate(output, input, inputSize));
        std::memcpy(IV, output + inputSize - 16, 16);
    }
}
