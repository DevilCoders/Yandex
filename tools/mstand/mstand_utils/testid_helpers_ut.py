import mstand_utils.testid_helpers as utestid


# noinspection PyClassHasNoInit
class TestTestidChecking:
    def test_simple(self):
        assert utestid.testid_is_simple("123")
        assert utestid.testid_is_simple("99999")
        assert not utestid.testid_is_simple("")
        assert not utestid.testid_is_simple("0")
        assert not utestid.testid_is_simple(" 123")
        assert not utestid.testid_is_simple("10o0")
        assert not utestid.testid_is_simple("all")
        assert not utestid.testid_is_simple("bromozel_test")

    def test_all(self):
        assert utestid.testid_is_all("all")
        assert utestid.testid_is_all("0")
        assert not utestid.testid_is_all("")
        assert not utestid.testid_is_all("1234")

    def test_adv(self):
        assert utestid.testid_is_adv("a_1234")
        assert utestid.testid_is_adv("a_0")
        assert not utestid.testid_is_adv("a_")
        assert not utestid.testid_is_adv("1234")
        assert not utestid.testid_is_adv("")
        assert utestid.convert_adv_testid("a_1234") == "1234"

    def test_adminka(self):
        assert utestid.testid_from_adminka("1234")
        assert not utestid.testid_from_adminka("all")
        assert not utestid.testid_from_adminka("a_1234")
        assert not utestid.testid_from_adminka("a_0")
        assert not utestid.testid_from_adminka("0")
        assert not utestid.testid_from_adminka("rec:ab:desktop_redesign_final:default")
        assert not utestid.testid_from_adminka("all")
        assert not utestid.testid_from_adminka("a_")
        assert not utestid.testid_from_adminka("")
        assert not utestid.testid_from_adminka("ab:desktop_redesign_final:desktop_redesign_subscriptions_feed")
