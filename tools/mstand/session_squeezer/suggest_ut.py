# coding=utf-8

import session_squeezer.suggest


# noinspection PyClassHasNoInit
class TestSuggestTpahLog:
    def test_normal(self):
        tpah_log = "[[clear,p0,0],[clear,p0,505],[add,p0,6002]]"
        expected = [["clear", "p0", 0], ["clear", "p0", 505], ["add", "p0", 6002]]
        parsed = session_squeezer.suggest.parse_tpah_log(tpah_log)
        assert parsed == expected

    def test_encoded(self):
        tpah_log = "%5B%5Bclear,p0,0%5D,%5Bclear,p0,593%5D,%5Badd,p0,3863%5D%5D"
        expected = [["clear", "p0", 0], ["clear", "p0", 593], ["add", "p0", 3863]]
        parsed = session_squeezer.suggest.parse_tpah_log(tpah_log)
        assert parsed == expected

    def test_empty(self):
        assert session_squeezer.suggest.parse_tpah_log(None) is None
        assert session_squeezer.suggest.parse_tpah_log("") == []
        assert session_squeezer.suggest.parse_tpah_log("[]") == []

    def test_bad_int(self):
        tpah_log = "[[clear,p0,0],[clear,p0,505],[add,p0,\\x7f]]"
        expected = [["clear", "p0", 0], ["clear", "p0", 505], ["add", "p0", "\\x7f"]]
        parsed = session_squeezer.suggest.parse_tpah_log(tpah_log)
        assert parsed == expected

    def test_bad_part(self):
        tpah_log = "[[clear,p0,0],[clear,p0,505],[add,p0]]"
        expected = [["clear", "p0", 0], ["clear", "p0", 505], ["add,p0"]]
        parsed = session_squeezer.suggest.parse_tpah_log(tpah_log)
        assert parsed == expected

    def test_error_tpah(self):
        tpah_log = "[[NTZ,%C2%86%C2%8C,%C2%B6],["
        assert session_squeezer.suggest.parse_tpah_log(tpah_log) is None
