import pytest

import reports.report_helpers as rhelp


# noinspection PyClassHasNoInit
class TestVerdicts(object):
    good_tags_old = [
        "gp_verdict_3_absolutely",
    ]

    bad_tags_invalid_format = [
        "gp_verdict_3",
        "gp_verdict_34246",
    ]

    bad_tags_duplicates = [
        "gp_verdict_1_maybe",
        "gp_verdict_3_absolutely",
    ]

    bad_tags_no_verdict = [
        "gp_verdict_maybe",
    ]

    good_tags_new = [
        "gp_verdict_3_absolutely_12345",
    ]

    bad_tags_duplicate_testid = [
        "gp_verdict_3_absolutely_54321",
        "gp_verdict_3_absolutely",
    ]

    tags_bad_and_good = [
        "gp_verdict_1_qwe_1001",
        "verdict_bad_2_rty_1002",
        "verdict_good_3_uio_1003",
    ]

    def test_old_notation(self):
        assert rhelp.get_verdict_by_testid(self.good_tags_old, 0).bad == 3
        assert rhelp.get_verdict_by_testid(None, 0).is_unknown()
        assert rhelp.get_verdict_by_testid([], 0).is_unknown()
        assert rhelp.get_verdict_by_testid(self.bad_tags_invalid_format, 0).is_unknown()
        assert rhelp.get_verdict_by_testid(self.bad_tags_no_verdict, 0).is_unknown()

        with pytest.raises(Exception, match="Duplicate verdicts in testid 0"):
            rhelp.get_verdict_by_testid(self.bad_tags_duplicates, 0)

    def test_new_notation(self):
        assert rhelp.get_verdict_by_testid(self.good_tags_new, 12345).bad == 3
        assert rhelp.get_verdict_by_testid(self.good_tags_new, 54321).is_unknown()
        assert rhelp.get_verdict_by_testid(self.good_tags_new, 11111).is_unknown()

        with pytest.raises(Exception, match="Duplicate verdicts in testid 54321"):
            rhelp.get_verdict_by_testid(self.bad_tags_duplicate_testid, 54321)

    def test_good_verdicts(self):
        assert rhelp.get_verdict_by_testid(self.tags_bad_and_good, 1001).bad == 1
        assert rhelp.get_verdict_by_testid(self.tags_bad_and_good, 1002).bad == 2
        assert rhelp.get_verdict_by_testid(self.tags_bad_and_good, 1003).good == 3


# noinspection PyClassHasNoInit
class TestAspects(object):
    good_tags = [
        "gp_aspect_ranking_newformula",
        "gp_aspect_ranking"
    ]

    bad_tags_duplicates = [
        "gp_aspect_ranking_newformula",
        "gp_aspect_ranking_personalization",
        "gp_aspect_ranking"
    ]

    def test_aspects(self):
        parent, child = rhelp.get_aspects(self.good_tags)
        assert parent == "ranking"
        assert child == "newformula"

        assert rhelp.get_aspects(None) is None

        with pytest.raises(Exception, match=r"Only one aspect per observation.*"):
            rhelp.get_aspects(self.bad_tags_duplicates)
