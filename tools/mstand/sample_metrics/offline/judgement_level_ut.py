import sample_metrics.offline.judgement_level as mjl


class ResultInfoForAPIMock(object):
    def __init__(self, scales):
        self.scales = scales

    def has_scale(self, scale):
        return scale in self.scales


# noinspection PyClassHasNoInit
class TestJudgementLevel:
    def test_result_has_scale_str(self):
        result = ResultInfoForAPIMock({"scale1": 1.0, "scale2": 2.0})
        assert mjl.result_has_scales(result, "scale1")
        assert mjl.result_has_scales(result, "scale2")
        assert not mjl.result_has_scales(result, "scale3")

    def test_result_has_scale_list(self):
        result = ResultInfoForAPIMock({"scale1": 1.0, "scale2": 2.0})
        assert mjl.result_has_scales(result, ["scale1", "scale2"])
        assert not mjl.result_has_scales(result, ["scale1", "scale3"])
        assert not mjl.result_has_scales(result, ["scale2", "scale3"])

    def test_result_has_scale_dict(self):
        result = ResultInfoForAPIMock({"scale1": 1.0, "scale2": 2.0, "scale": {"nested_scale": 3.0}})
        assert mjl.result_has_scales(result, {"scale": "nested_scale"})
        assert mjl.result_has_scales(result, {"scale": ["nested_scale"]})
        assert mjl.result_has_scales(result, ["scale1", "scale2", {"scale": "nested_scale"}])
        assert not mjl.result_has_scales(result, ["scale1", "scale2", {"scale": "nested_scale_x"}])
        assert not mjl.result_has_scales(result, ["scale1", "scale2", {"scale_x": "nested_scale"}])
