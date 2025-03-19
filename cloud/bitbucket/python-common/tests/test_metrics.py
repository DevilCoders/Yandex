import math
import pytest

from yc_common import metrics, config
from yc_common.exceptions import Error
from yc_common.test.core import monkeypatch_function

DEFAULT_LABELS = {"service": "compute", "region": "man1"}


@pytest.fixture
def empty_config(monkeypatch):
    monkeypatch_function(monkeypatch, config.get_value, {})


@pytest.fixture
def config_with_values(monkeypatch):
    monkeypatch_function(monkeypatch, config.get_value, DEFAULT_LABELS)


def sample_count(m):
    s = list(m.metric._samples())
    return len(s)


def sample_value(m):
    return list(m.metric._samples())[0][2]


def sample_labels(m):
    return list(m.metric._samples())[0][1]


def test_exponential_buckets():
    _buckets = (1.0, 10.0, 100.00, 1000.0, 10000.0, float("inf"))
    _generated = metrics.exponential_buckets(1, 10, 5)
    assert len(_buckets) == len(_generated)
    assert all(math.isclose(b1, b2) for b1, b2 in zip(_buckets, _generated))


def test_unknown_metric_type():
    with pytest.raises(Error):
        metrics.Metric("UNKNOWN", "m1", ["label"])


def test_metric_registry(empty_config):
    c1 = metrics.Metric(metrics.MetricTypes.COUNTER, "cA", ["labelA1", "labelA2"])
    c1.labels("a1", "a2").inc()

    c2 = metrics.Metric(metrics.MetricTypes.COUNTER, "cA", ["labelA1", "labelA2"])
    c2.labels("a1", "a2").inc()

    assert sample_count(c2) == 2
    assert math.isclose(sample_value(c1), 2.0)


def test_metric_label_values(empty_config):
    c = metrics.Metric(metrics.MetricTypes.GAUGE, "x", ["a1", "b1"])
    c.labels("y", "z").inc()

    labels = sample_labels(c)
    assert labels["a1"] == "y"
    assert labels["b1"] == "z"


def test_metric_registry_with_arbitrary_labels_order(empty_config):
    c1 = metrics.Metric(metrics.MetricTypes.COUNTER, "c", ["A", "B", "C"])
    c1.labels("a", "b", "c").inc()

    c2 = metrics.Metric(metrics.MetricTypes.COUNTER, "c", ["A", "C", "B"])
    c2.labels("a", "c", "b").inc()

    c3 = metrics.Metric(metrics.MetricTypes.COUNTER, "c", ["B", "C", "A"])
    c3.labels("b", "c", "a").inc()

    assert sample_count(c3) == 2
    assert math.isclose(sample_value(c3), 3.0)


def test_metric_with_wrong_labels(empty_config):
    c1 = metrics.Metric(metrics.MetricTypes.COUNTER, "cB", ["labelB1", "labelB2"])
    with pytest.raises(Error):
        c1.labels("b1").inc()


def test_metric_registry_with_default_labels(config_with_values):
    c1 = metrics.Metric(metrics.MetricTypes.COUNTER, "cC", ["labelC1", "labelC2"])
    c1.labels("c1", "c2").inc()

    c2 = metrics.Metric(metrics.MetricTypes.COUNTER, "cC", ["labelC2", "labelC1"])
    c2.labels("c2", "c1").inc()

    assert sample_count(c2) == 2

    samples = metrics.collect(encode=metrics.MetricFormat.JSON)
    assert all((label in samples["cC"][0][0]) for label in ["labelC1", "labelC2"] + list(DEFAULT_LABELS))


def test_metric_registry_with_no_labels(empty_config):
    c1 = metrics.Metric(metrics.MetricTypes.COUNTER, "cD")
    c1.inc()

    assert sample_count(c1) == 2
    assert math.isclose(sample_value(c1), 1.0)


def test_metric_registry_with_default_labels_only(config_with_values):
    c = metrics.Metric(metrics.MetricTypes.COUNTER, "cE")
    c.inc()

    assert sample_count(c) == 2

    samples = metrics.collect(encode=metrics.MetricFormat.JSON)
    assert all((label in samples["cE"][0][0]) for label in list(DEFAULT_LABELS))


def test_metric_registry_duplicate_error(empty_config):
    c = metrics.Metric(metrics.MetricTypes.GAUGE, "cF", ["label"])
    c.labels("a").inc()

    with pytest.raises(Error):
        metrics.Metric(metrics.MetricTypes.COUNTER, "cF", ["label"]).labels("b").inc()
    c.labels("a").inc()

    with pytest.raises(Error):
        metrics.Metric(metrics.MetricTypes.GAUGE, "cF", ["another_label"]).labels("b").inc()
    c.labels("a").inc()

    assert sample_count(c) == 1
    assert math.isclose(sample_value(c), 3.0)


def test_histogram_metric(empty_config):
    h1 = metrics.Metric(metrics.MetricTypes.HISTOGRAM, "h1", ["label"])
    for value in range(10):
        h = metrics.Metric(metrics.MetricTypes.HISTOGRAM, "h1", ["label"])
        h.labels("a").observe(value)
    assert math.isclose(h1.labels("a")._sum.get(), 45.0)


def test_metric_registry_reset(empty_config):
    c = metrics.Metric(metrics.MetricTypes.COUNTER, "cJ", ["label"])
    c.labels("one").inc()

    samples = metrics.collect(encode=metrics.MetricFormat.JSON)
    assert "cJ" in samples
    assert math.isclose(sample_value(c), 1.0)

    assert metrics.reset("cJ")
    assert "cJ" not in metrics.collect(encode=metrics.MetricFormat.JSON)
