"""
Test for register
"""

import pytest

from dbaas_internal_api.modules.postgres.constants import MY_CLUSTER_TYPE as PG_CLUSTER_TYPE
from dbaas_internal_api.utils.register import DbaasOperation, Resource, UnsupportedHandlerError, get_request_handler

# pylint: disable=invalid-name, missing-docstring


class Test_get_request_handler:
    def test_return_existed(self):
        assert get_request_handler(PG_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE) is not None

    def test_raise_for_non_existed_operation(self):
        with pytest.raises(UnsupportedHandlerError):
            get_request_handler(PG_CLUSTER_TYPE, Resource.HOST, DbaasOperation.CREATE_BACKUP)

    def test_raise_for_non_existed_cluster_type(self):
        with pytest.raises(UnsupportedHandlerError):
            get_request_handler('YA RLY!', Resource.HOST, DbaasOperation.BATCH_CREATE)
