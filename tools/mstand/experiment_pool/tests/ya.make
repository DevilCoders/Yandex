PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/experiment_pool/criteria_result_ut.py
    tools/mstand/experiment_pool/conftest.py
    tools/mstand/experiment_pool/deserializer_ut.py
    tools/mstand/experiment_pool/experiment_for_calc_ut.py
    tools/mstand/experiment_pool/experiment_ut.py
    tools/mstand/experiment_pool/filter_metric_ut.py
    tools/mstand/experiment_pool/meta_ut.py
    tools/mstand/experiment_pool/metric_result_ut.py
    tools/mstand/experiment_pool/observation_ut.py
    tools/mstand/experiment_pool/pool_helpers_ut.py
    tools/mstand/experiment_pool/pool_ut.py
)

PEERDIR(
    tools/mstand/experiment_pool
    tools/mstand/mstand_structs
    tools/mstand/user_plugins
)

DATA(
    arcadia/tools/mstand/experiment_pool/tests/data/v0/basic_input.json
    arcadia/tools/mstand/experiment_pool/tests/data/v0/basic_long.json
    arcadia/tools/mstand/experiment_pool/tests/data/v0/empty.json
    arcadia/tools/mstand/experiment_pool/tests/data/v0/repeated_observations.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/basic_input.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/basic_long.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/merge_base.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/merge_experiment.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/merge_metric_result.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/merge_pool.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/pool_abt.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/pool_with_criteria_results.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/pool_with_lamps.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/same_metric_twice.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/same_metric_twice_different_exps.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/simple_pool.json
    arcadia/tools/mstand/experiment_pool/tests/data/v1/very_minimal_pool.json
    arcadia/tools/mstand/experiment_pool/tests/data/enumerate_all_metrics_in_observations.json
    arcadia/tools/mstand/experiment_pool/tests/data/enumerate_empty_lamps_in_observations.json
    arcadia/tools/mstand/experiment_pool/tests/data/pool_for_merge_1.json
    arcadia/tools/mstand/experiment_pool/tests/data/pool_for_merge_2.json
    arcadia/tools/mstand/experiment_pool/tests/data/pool_to_convert.json
    arcadia/tools/mstand/experiment_pool/tests/data/pool_to_convert_zero_control.json
)

SIZE(SMALL)

END()
