namespace NHostBoost;

struct TParams {
    Enabled : bool (default = false);
    Rules : struct[] {
        Host : string (required);
        Grouping : string (required);
        MoveRules : struct[] {
            // Check only first TopSize documents to find requested host
            TopSize : ui32 (required);

            // Move host to exact position
            ExactPos : ui32;

            // Pull up document on some positions
            PullUpPos : ui32;
            // Do not set document before this position
            MaxDocumentPos : ui32;
        };
    };
};
