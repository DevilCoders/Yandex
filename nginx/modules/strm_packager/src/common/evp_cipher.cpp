#include <nginx/modules/strm_packager/src/common/evp_cipher.h>

namespace NStrm::NPackager {
    TEvpCipher::TEvpCipher(EVP_CIPHER const* type)
        : Type_(type ? type : EVP_enc_null())
        , KeyLength_(EVP_CIPHER_key_length(Type_))
        , IVLength_(EVP_CIPHER_iv_length(Type_))
        , BlockSize_(EVP_CIPHER_block_size(Type_))
        , Context_(nullptr)
        , DirectionInit_(EDirection::Unset)
    {
        Y_ENSURE(type);
    }

    void TEvpCipher::Init(const EDirection dir, ui8 const* key, ui8 const* iv, bool padding) {
        Y_ENSURE(dir == EDirection::Encrypt || dir == EDirection::Decrypt);
        Y_ENSURE(key);
        Y_ENSURE(iv);
        if (!Context_) {
            Context_ = EVP_CIPHER_CTX_new();
            Y_ENSURE(Context_);
        }
        DirectionInit_ = EDirection::Unset;

        const auto init = (dir == EDirection::Encrypt) ? EVP_EncryptInit_ex : EVP_DecryptInit_ex;

        Y_ENSURE(1 == init(Context_, Type_, nullptr, key, iv));
        Y_ENSURE(1 == EVP_CIPHER_CTX_set_padding(Context_, padding ? 1 : 0));
        DirectionInit_ = dir;
        Padding_ = padding;
        Finalized_ = false;
        InputSizeCounter_ = 0;
        OutputSizeCounter_ = 0;
    }

    size_t TEvpCipher::Update(const EDirection dir, ui8* output, ui8 const* input, size_t inputSize) {
        Y_ENSURE(output);
        Y_ENSURE(input);
        Y_ENSURE(inputSize > 0);
        Y_ENSURE(Context_ && dir == DirectionInit_);

        const auto update = (dir == EDirection::Encrypt) ? EVP_EncryptUpdate : EVP_DecryptUpdate;

        int outputSize = 0;
        Y_ENSURE(1 == update(Context_, output, &outputSize, input, inputSize));

        InputSizeCounter_ += inputSize;
        OutputSizeCounter_ += outputSize;

        return outputSize;
    }

    size_t TEvpCipher::Final(const EDirection dir, ui8* output) {
        Y_ENSURE(output);
        Y_ENSURE(Context_ && dir == DirectionInit_);
        Y_ENSURE(!Finalized_);

        const auto final = (dir == EDirection::Encrypt) ? EVP_EncryptFinal_ex : EVP_DecryptFinal_ex;

        int outputSize = 0;
        Y_ENSURE(1 == final(Context_, output, &outputSize));
        OutputSizeCounter_ += outputSize;
        Finalized_ = true;

        return outputSize;
    }

    TEvpCipher::~TEvpCipher() {
        if (Context_) {
            EVP_CIPHER_CTX_free(Context_);
        }
    }

}
