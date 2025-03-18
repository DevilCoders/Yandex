namespace NVoiceSearch;

struct TIntentConfig {
    ThresholdPosition : ui32 (default = 5);
    ThresholdWeight : double (default = 0.2);
    UseWithLocalRandom : bool (default = false);
    Markers : [string];
    AntiMarkers: [string];
};

struct TVoiceSearchConfig {
    IntentConfigs : {string -> TIntentConfig};
};
