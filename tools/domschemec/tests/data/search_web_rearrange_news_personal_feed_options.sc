namespace NNewsPersonalFeed;

struct TConfig {
    Enabled            : bool (default = false);
    Top                : ui32 (default = 0);
    TabTitle           : string (default = "Рекомендуем");
    TitleLen           : ui32 (default = 150);
    CanShowInSideBlock : bool (default = false);
    AllowEmptyImages   : bool (default = false);
    MaxFeedDocs        : ui32 (default = 5);
    MinFeedDocs        : ui32 (default = 5);
    IsActiveTab        : bool (default = false);
    TabIndex           : ui32 (default = 0);
};
