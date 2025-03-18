import os

from io import StringIO

import experiment_pool.pool_helpers as phelp
import reports.sensitivity_compare as rsc


def get_pool_path(name, root_path):
    return os.path.join(root_path, "tests/ut_data/compare_sensitivity", name)


# noinspection PyClassHasNoInit
class TestCompareSensitivity:
    def test_compare_observations(self, root_path):
        # here two real large pools were taken for simplicity.
        # TODO: replace them with 'synthetic' data
        pool1_path = get_pool_path("pool1.json", root_path)
        pool2_path = get_pool_path("pool2.json", root_path)
        pools = [pool1_path, pool2_path]
        merged_pool = phelp.load_and_merge_pools(pools)

        # ujson.dump(merged_pool.serialize())
        metric_key_id_map = phelp.generate_metric_key_id_map(merged_pool)

        list(rsc.generate_sensitivity_table(merged_pool, metric_key_id_map, min_pvalue=0.01, threshold=0.01))
        # TODO: validate result

    def test_render_sensitivity_table(self, root_path):
        pool1_path = get_pool_path("pool1.json", root_path)
        pool2_path = get_pool_path("pool2.json", root_path)
        pools = [pool1_path, pool2_path]
        merged_pool = phelp.load_and_merge_pools(pools)
        metric_key_id_map = phelp.generate_metric_key_id_map(merged_pool)
        rows = rsc.generate_sensitivity_table(merged_pool, metric_key_id_map, min_pvalue=0.01, threshold=0.01)

        metric_id_key_map = phelp.reverse_metric_id_key_map(metric_key_id_map)
        stats = rsc.collect_stats(rows)
        template_dir = os.path.join(root_path, "templates")
        rsc.render_template(
            name='wiki_sensitivity.tpl',
            rows=rows,
            metric_id_key_map=metric_id_key_map,
            stats=stats,
            min_pvalue=0.000001,
            threshold=0.01,
            template_dir=template_dir,
        )
        rsc.render_template(
            name='html_sensitivity.tpl',
            rows=rows,
            metric_id_key_map=metric_id_key_map,
            stats=stats,
            min_pvalue=0.000001,
            threshold=0.01,
            template_dir=template_dir,
        )

        tsv_out = StringIO()
        rsc.write_tsv(tsv_out, rows, metric_id_key_map)
        rows = [
            "Metric\tSensitivity\tkstest\tpvalue_count\tmetric_one\tmetric_two",
            "metric_one\t3.1365\t0.000000\t74\t\tgray 0.1258 0.2202 7.55%",
            "metric_two\t2.9163\t0.000000\t74\tgray 0.1258 -0.2202 -7.02%",
        ]

        assert tsv_out.getvalue().strip() == "\n".join(rows)
