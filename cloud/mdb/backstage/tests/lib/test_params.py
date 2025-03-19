import pytest

import cloud.mdb.backstage.lib.params as mod_params


@pytest.mark.parametrize(
    "param_value,param_result",
    [
        (1, {'result': 1, 'err': None}),
        ("1", {'result': 1, 'err': None}),
        ("abc", {'result': None, 'err': "failed to parse int: invalid literal for int() with base 10: 'abc'"}),
    ],
)
def test_validate_int(param_value, param_result):
    result, err = mod_params.validate_int(param_value)
    assert result == param_result['result']
    assert err == param_result['err']
