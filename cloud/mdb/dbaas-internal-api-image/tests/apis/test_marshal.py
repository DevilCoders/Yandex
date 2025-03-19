"""
tests for our parse_kwargs utils
"""
import pytest

from dbaas_internal_api.apis import marshal
from dbaas_internal_api.utils.register import DbaasOperation, Resource

# pylint: disable=invalid-name, missing-docstring


class Test_marshal_with_resources:
    def test_fail_for_unsupported_cluster_type(self):
        @marshal.with_resource(Resource.CLUSTER, DbaasOperation.INFO)
        def tgt(**_):
            pass

        with pytest.raises(RuntimeError):
            tgt()


# positive cases checked in func tests
