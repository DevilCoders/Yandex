namespace NDistributionConveyor;

struct TTargetFunctionConfig {
    expression(required): string;
};

struct TFilterConfig {
    show_id_regex: string(default = ".*");
    host_regex: string(default = ".*");
    referer_regex: string(default = ".*");
    vars_regex: string(default = ".*");
};

struct TExperimentalFeaturesConfig {
    enabled: bool(default = false);
};

struct TFmlConfig {
    target_function_config(required): TTargetFunctionConfig;
    filter_config(required): TFilterConfig;
    experimental_features_config: TExperimentalFeaturesConfig;
    fml_name(required): string;
    iterations: ui32;
    non_training_zero_fraction: double(default = 0);
    non_training_non_zero_fraction: double(default = 0);
};

struct TConfig {
    fml_configs(required): [TFmlConfig];
    exp_hosts: [string];
};
