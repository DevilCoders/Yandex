namespace NBlender::NWizardsClicksSc;

struct TDynamicCalculatorConfig {
    Path (required) : string;
    NavigPath : string;
    UseAggrUI : bool (default = true);
    MinShows: i32 (default = 30);
    UseNa: bool (default = true);
    FeatureSet: string (default = "all");
};
