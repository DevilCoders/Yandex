local _M = {_VERSION = '1.0'}

local ffi = require "ffi"

ffi.cdef[[

    typedef struct engine_st ENGINE;

    typedef struct evp_cipher_ctx_st EVP_CIPHER_CTX;
    typedef struct evp_cipher_st EVP_CIPHER;

    EVP_CIPHER_CTX *EVP_CIPHER_CTX_new();
    void EVP_CIPHER_CTX_free(EVP_CIPHER_CTX *a);

    const EVP_CIPHER *EVP_aes_128_ecb(void);
    const EVP_CIPHER *EVP_aes_192_ecb(void);
    const EVP_CIPHER *EVP_aes_256_ecb(void);

    int EVP_CIPHER_CTX_set_padding(EVP_CIPHER_CTX *x, int padding);
    int EVP_DecryptInit_ex(EVP_CIPHER_CTX *ctx, const EVP_CIPHER *cipher, ENGINE *impl, unsigned char *key, const unsigned char *iv);
    int EVP_DecryptUpdate(EVP_CIPHER_CTX *ctx, unsigned char *out, int *outl, const unsigned char *in, int inl);
    int EVP_DecryptFinal_ex(EVP_CIPHER_CTX *ctx, unsigned char *outm, int *outl);

]]

--[[
Реализация дешифрования, используемого в инвертированной схеме
    cookie_value -  бинарная строка, содержащая подготовленные для расшифровки данные
                    Длина строки должна быть кратна 16 (размеру блока шифра)
    key -           бинарная строка, содержащая ключ блоного шифра
                    Размер ключа должен быть равен ровно 16 байтам
]]
function _M.decrypt_cookie(cookie_value, key)
    local ciper_length = 128

    if not cookie_value or #cookie_value % (ciper_length / 8) ~= 0 then
        ngx.log(ngx.ERR, "AntiADB-", ngx.var.request_id, " ERROR: AAB cookie value has incorrect length")
        return nil
    end

    if not key or #key ~= 16 then
        ngx.log(ngx.ERR, "AntiADB-", ngx.var.request_id, " ERROR: parnter key has incorrect length")
        return nil
    end

    local data_len = #cookie_value
    local context = ffi.C.EVP_CIPHER_CTX_new()
    if context == nil
    then
        ngx.log(ngx.ERR, "AntiADB-", ngx.var.request_id, " ERROR: failed to create cryptographic context. Not enough memory?")
        return nil
    end
    context = ffi.gc(context, ffi.C.EVP_CIPHER_CTX_free)

    local out_buf = ffi.new("unsigned char[?]", data_len)
    local out_len = ffi.new("int[1]")
    local bin_key = ffi.new("unsigned char[?]", ciper_length/8)
    ffi.copy(bin_key, key, ciper_length/8)

    if     ffi.C.EVP_DecryptInit_ex(context, ffi.C.EVP_aes_128_ecb(), nil, bin_key, nil) ~= 1
        or ffi.C.EVP_DecryptUpdate(context, out_buf, out_len, cookie_value, data_len) ~= 1
        -- There we do not need EVP_DecryptFinal_ex, because padding implemented at upper level
    then
        ngx.log(ngx.ERR, "AntiADB-", ngx.var.request_id, " ERROR: error during decryption surfer cookie")
        return nil
    end

    local result = ffi.string(out_buf, data_len)
    return result
end

return _M
