namespace NQuerySearch;

struct TSaaSKeyTypeOptions {
    Disable : bool;
    // true - по последнему измерению ключа должно находится не более 1 документа
    LastUnique: bool (default = false);
};

// Описание сервиса SaaS, поддерживающего формат kernel/querydata/saas
struct TSaaSServiceOptions {
        // Описание типов ключей, которые необходимо сгенерировать, см. kernel/querydata/saas/qd_saas_key.h
        // Пример: {QueryStrong:{}, QueryDoppel-Owner:{Disable:1}}
        // - включить генерацию QueryStrong, отключить генерацию пар QueryDoppel+Owner
    KeyTypes : {string -> TSaaSKeyTypeOptions};

        // Максимальное число ключей в одном запросе.
        // Много ключей в запросе - большое время ответа. Мало ключей - много параллельных запросов в метапоиске.
    MaxKeysInRequest : ui32 (default = 32, > 0);

        // Максимальное число порождаемых запросов.
        // Ограничение MaxRequests выигрывает у MaxKeysInRequests.
    MaxRequests : ui32 (default = 32, > 0);

        // kps по умолчанию
    DefaultKps : ui64 (default = 1);

        // KV либо Trie
    SaaSType : string (default = "None");

        // true - объединяются типы ключей, которые являются префиксами друг друга
        // например, Url можно "подклеить" к Url+Region
        // так же в этом режиме масочные ключи расщепляются на стороне SAAS
    OptimizeKeyTypes: bool (default = false);
};
