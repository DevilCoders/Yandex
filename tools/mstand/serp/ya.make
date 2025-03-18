PY3_LIBRARY()

OWNER(
    g:mstand
)

PY_SRCS(
    NAMESPACE serp
    __init__.py
    lock_struct.py
    data_storages.py
    extender_context.py
    extender_key.py
    field_extender.py
    field_extender_api.py
    json_lines_ops.py
    keys_dumper.py
    markup_struct.py
    parsed_serp_struct.py
    mc_calc_options.py
    mc_composite.py
    mc_results.py
    offline_calc_contexts.py
    offline_metric_caps.py
    query_struct.py
    scale_stats.py
    serp_attrs.py
    serp_compute_metric.py
    serp_compute_metric_single.py
    serp_compute_metric_single_ut.py
    serp_fetch_params.py
    serp_helpers.py
    serpset_converter.py
    serpset_fetcher.py
    serpset_fetcher_single.py
    serpset_metric_values.py
    serpset_ops.py
    serpset_parse_contexts.py
    serpset_parser.py
    serpset_parser_single.py
)

PEERDIR(
    contrib/python/pytest
    quality/yaqlib/yaqutils
    quality/yaqlib/pytlib
    quality/yaqlib/yaqschemas
    quality/yaqlib/yaqlibenums
    tools/mstand/experiment_pool
    tools/mstand/mstand_enums
)

END()

RECURSE_FOR_TESTS(
    tests
)
