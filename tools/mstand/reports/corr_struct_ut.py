from reports import CorrelationResult
from user_plugins import PluginKey

import yaqutils.json_helpers as ujson


# noinspection PyClassHasNoInit
class TestCorrelationResult:
    def test_sort_corr_results(self):
        mk1 = PluginKey("metric1")
        mk2 = PluginKey("metric2")

        c1 = CorrelationResult(mk1, mk2, 0.5)
        c2 = CorrelationResult(mk1, mk2, 0.3)
        c3 = CorrelationResult(mk1, mk2, 0.1)
        c4 = CorrelationResult(mk1, mk2, 0.7)
        c5 = CorrelationResult(mk1, mk2, 0.6)

        sorted_results = CorrelationResult.sorted_by_value([c1, c2, c3, c4, c5])
        assert sorted_results[0].corr_value == 0.7
        assert sorted_results[2].corr_value == 0.5
        assert sorted_results[4].corr_value == 0.1

    def test_corr_result_serialize(self):
        mk1 = PluginKey("metric1")
        mk2 = PluginKey("metric2")

        corr_res_single = CorrelationResult(mk1, mk2, corr_value=0.314)
        serialized = corr_res_single.serialize()
        ujson.dump_to_str(serialized)
        assert CorrelationResult.deserialize(serialized) == corr_res_single

        corr_res_tuple = CorrelationResult(mk1, mk2, corr_value=(0.95, 0.0001))
        ujson.dump_to_str(corr_res_tuple.serialize())

        corr_res_nan = CorrelationResult(mk1, mk2, corr_value=float('nan'))
        ujson.dump_to_str(corr_res_nan.serialize())

        corr_res_nan = CorrelationResult(mk1, mk2, corr_value=None)
        ujson.dump_to_str(corr_res_nan.serialize())

    def test_corr_tsv_length(self):
        mk1 = PluginKey("metric1")
        mk2 = PluginKey("metric2")

        c1 = CorrelationResult(mk1, mk2, 0.5)
        check_tsv_line(c1)

        c2 = CorrelationResult(mk1, mk2, 0.3, 1.5, 2.5)
        check_tsv_line(c2)

        c3 = CorrelationResult(mk1, mk2, 0.1, additional_info="hello world 1")
        check_tsv_line(c3)

        c4 = CorrelationResult(mk1, mk2, 0.7, additional_info="hello\tworld\n2")
        check_tsv_line(c4)

    def test_corr_result_operators(self):
        mk1 = PluginKey("metric1")
        mk2 = PluginKey("metric2")

        corr_one = CorrelationResult(mk1, mk2, 0.765)
        corr_same = CorrelationResult(mk1, mk2, 0.765)

        corr_two = CorrelationResult(mk1, mk2, 0.432)
        assert corr_one == corr_same
        assert corr_one != corr_two

        assert "0.765" in str(corr_one)


def check_tsv_line(corr_result):
    header = parse_tsv_line(CorrelationResult.tsv_header())
    row = parse_tsv_line(corr_result.tsv_line())
    assert len(header) == len(row)


def parse_tsv_line(line):
    assert "\n" not in line
    return line.split("\t")
