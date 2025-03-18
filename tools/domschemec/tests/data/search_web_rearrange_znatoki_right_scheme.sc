namespace NZnatokiRight;

struct TParams {
    Enabled : bool (default = false);

    IntentName : string (default = "WEB_ZNATOKI");
    MaxIntentPos : ui64 (default = 15);
    ZnatokiUrlString : string (default = "yandex.ru/znatoki");
    ClearRightBeforeAdd : bool (default = false);
    WorkOnlyIfEmptyRight : bool (default = true);
};
