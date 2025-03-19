"""Test misc utils"""

import pytest

from yc_common.exceptions import LogicalError
from yc_common.misc import safe_zip


def test_safe_zip():
    assert list(safe_zip((1, 2), (10, 20))) == [(1, 10), (2, 20)]

    with pytest.raises(LogicalError):
        assert list(safe_zip((1,), (10, 20)))

    with pytest.raises(LogicalError):
        assert list(safe_zip((1, 2), (10,)))
