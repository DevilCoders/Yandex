PY3TEST()

OWNER(
    g:mstand
)

TEST_SRCS(
    tools/mstand/serp/conftest.py
    tools/mstand/serp/field_extender_ut.py
    tools/mstand/serp/json_lines_ops_ut.py
    tools/mstand/serp/markup_struct_ut.py
    tools/mstand/serp/mc_composite_ut.py
    tools/mstand/serp/serp_compute_metric_single_ut.py
    tools/mstand/serp/serp_compute_metric_ut.py
    tools/mstand/serp/serpset_converter_ut.py
    tools/mstand/serp/serpset_parser_ut.py
)

PEERDIR(
    tools/mstand/metrics_api
    tools/mstand/mstand_utils
    tools/mstand/serp
    tools/mstand/sample_metrics
)

DATA(
    arcadia/tools/mstand/debug_data/offline/detailed_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/detailed_with_depth_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/detailed_with_precompute_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/judgement_level_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/nan_inf_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/not_numbers_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/nothing_metric/metric_external/metric_external_serpset_100500.json
    arcadia/tools/mstand/debug_data/offline/nothing_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/params_fields_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/parsed_serpsets/markup_100500.jsonl
    arcadia/tools/mstand/debug_data/offline/parsed_serpsets/queries_100500.jsonl
    arcadia/tools/mstand/debug_data/offline/parsed_serpsets/urls_100500.jsonl
    arcadia/tools/mstand/debug_data/offline/precompute_serpset_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/rel_sum_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/url_len_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/yield_metric/metric_results/metric_0_serpset_100500.tsv
    arcadia/tools/mstand/debug_data/offline/bool_metric/metric_results/metric_0_serpset_100500.tsv
)

SIZE(SMALL)

END()
