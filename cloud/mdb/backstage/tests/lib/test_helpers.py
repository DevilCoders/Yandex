import pytest

import cloud.mdb.backstage.lib.helpers as mod_helpers


def test_action_result():
    with pytest.raises(ValueError):
        mod_helpers.ActionResult()

    assert bool(mod_helpers.ActionResult(error='1')) is False
    assert bool(mod_helpers.ActionResult(message='1')) is True
