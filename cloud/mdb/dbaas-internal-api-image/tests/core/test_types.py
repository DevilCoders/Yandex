"""
Test for types
"""

import pytest

from dbaas_internal_api.core.types import OperationStatus

# pylint: disable=invalid-name, missing-docstring


@pytest.mark.parametrize('st', [OperationStatus.done, OperationStatus.failed])
def test_terminal_operation_status(st):
    assert st.is_terminal()


@pytest.mark.parametrize(
    'st',
    [
        OperationStatus.running,
        OperationStatus.pending,
    ],
)
def test_not_terminal_operation_status(st):
    assert not st.is_terminal()
