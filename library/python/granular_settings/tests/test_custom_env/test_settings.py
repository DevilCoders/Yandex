# encoding: utf-8


def test_empty_custom_env():
    from .app.settings_empty_env import PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6

    assert PARAM1 == 'VALUE1_MAIN'
    assert PARAM2 == 'VALUE2_DEV'
    assert PARAM3 == 'VALUE3_MAIN'
    assert PARAM4 == 'VALUE4_MAIN'
    assert PARAM5 == 'VALUE5_MAIN'
    assert PARAM6 == 'VALUE6_MAIN'


def test_b2b_extra_local():
    from .app.settings_b2b_extra_local import PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7, PARAM8

    assert PARAM1 == 'VALUE1_MAIN'
    assert PARAM2 == 'VALUE2_DEV'
    assert PARAM3 == 'VALUE3_B2B_MAIN'
    assert PARAM4 == 'VALUE4_B2B_DEV'
    assert PARAM5 == 'VALUE5_EXTRA_MAIN'
    assert PARAM6 == 'VALUE6_EXTRA_DEV'
    assert PARAM7 == 'VALUE7_LOCAL_MAIN'
    assert PARAM8 == 'VALUE8_LOCAL_DEV'


def test_extra_local_b2b():
    from .app.settings_extra_local_b2b import PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7, PARAM8

    assert PARAM1 == 'VALUE1_MAIN'
    assert PARAM2 == 'VALUE2_DEV'
    assert PARAM3 == 'VALUE3_B2B_MAIN'
    assert PARAM4 == 'VALUE4_B2B_DEV'
    assert PARAM5 == 'VALUE5_EXTRA_MAIN'
    assert PARAM6 == 'VALUE6_EXTRA_DEV'
    assert PARAM7 == 'VALUE7_B2B_MAIN'
    assert PARAM8 == 'VALUE8_B2B_DEV'


def test_local_b2b_extra():
    from .app.settings_local_b2b_extra import PARAM1, PARAM2, PARAM3, PARAM4, PARAM5, PARAM6, PARAM7, PARAM8

    assert PARAM1 == 'VALUE1_MAIN'
    assert PARAM2 == 'VALUE2_DEV'
    assert PARAM3 == 'VALUE3_B2B_MAIN'
    assert PARAM4 == 'VALUE4_B2B_DEV'
    assert PARAM5 == 'VALUE5_EXTRA_MAIN'
    assert PARAM6 == 'VALUE6_EXTRA_DEV'
    assert PARAM7 == 'VALUE7_EXTRA_MAIN'
    assert PARAM8 == 'VALUE8_EXTRA_DEV'
