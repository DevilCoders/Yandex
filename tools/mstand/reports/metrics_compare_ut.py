import os

import experiment_pool.pool_helpers as pool_helpers
import reports.metrics_compare as rcm
from reports import ExperimentPair


def get_pool_path(name, root_path):
    return os.path.join(root_path, "tests/ut_data/compare_metrics", name)


def try_load_pool(name, root_path):
    return pool_helpers.load_pool(get_pool_path(name, root_path))


def render_pool(pool_file_name, session, root_path):
    pool_path = get_pool_path(pool_file_name, root_path)
    pool = pool_helpers.load_pool(pool_path)
    render_all_templates(pool, session, root_path)
    render_tsv(pool, session)


def render_all_templates(pool, session, root_dir):
    template_dir = os.path.join(root_dir, "templates")
    env = rcm.create_template_environment(pool=pool, session=session, validation_result=None,
                                          threshold=0.1, show_all_gray=True, show_same_color=True,
                                          template_dir=template_dir)
    templates = [
        'wiki_metrics.tpl',
        'wiki_metrics_vertical.tpl',
        'html_metrics_vertical.tpl'
    ]
    for template in templates:
        env.get_template(template).render()


def render_tsv(pool, session):
    list(rcm.build_tsv_rows(pool=pool, session=session, threshold=0.1))


# noinspection PyClassHasNoInit
class TestTemplates:
    def test_render_simple(self, session, root_path):
        render_pool("template_test_pool.json", session, root_path)

    def test_extra_metric_results(self, session, root_path):
        # experiments contain more metric results than control
        render_pool("extra_metric_results.json", session, root_path)

        # experiments contain less metric results than control
        render_pool("missing_metric_results.json", session, root_path)

    def test_zero_metric_values(self, session, root_path):
        # experiments with zero control values
        render_pool("zero_metric_values.json", session, root_path)

    def test_no_values(self, session, root_path):
        # no values in pool at all
        render_pool("no_metrics.json", session, root_path)

    def test_lamps(self, session, root_path):
        # values with lamps
        render_pool("metrics_lamps.json", session, root_path)


# noinspection PyClassHasNoInit
class TestHelperFunctions:
    def test_red_lamps_detector(self, root_path):
        pool = try_load_pool("metrics_lamps.json", root_path)
        exp_pairs = ExperimentPair.from_lamps(pool)
        assert rcm.detect_red_lamps(exp_pairs[:1])
        assert not rcm.detect_red_lamps(exp_pairs[1:])
