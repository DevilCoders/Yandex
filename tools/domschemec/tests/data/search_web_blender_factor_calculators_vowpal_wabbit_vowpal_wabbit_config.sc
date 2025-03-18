namespace NBlender::NVowpalWabbitConfig;

struct TVowpalWabbitConfig {
    ModelName (required) : string;
    UseQuery (required) : bool;
    UseQueryWithUid (required) : bool;
    QueryNgram : i32 (default = 1);
    UseDnorm (required) : bool;
    UseDnormWithUid (required) : bool;
    DnormNgram : i32 (default = 1);
    UseUid (required) : bool;
    UseUrl : bool (default = false);
    UniqueUrlWords : bool (default = false);
    IsIznanka : bool (default = false);
};
