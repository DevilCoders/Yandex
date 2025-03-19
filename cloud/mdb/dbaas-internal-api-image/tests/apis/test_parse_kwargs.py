"""
tests for our parse_kwargs utils
"""
import marshmallow
import pytest

from dbaas_internal_api.apis import parse_kwargs
from dbaas_internal_api.utils.register import DbaasOperation, Resource

# pylint: disable=invalid-name, missing-docstring


class SS(marshmallow.Schema):
    string = marshmallow.fields.Str(required=True)


class Test_parse_kwargs_with_schema:
    @pytest.mark.parametrize('bad', [SS(), Exception])
    def test_fail_for_schema_obj(self, bad):
        with pytest.raises(RuntimeError):
            parse_kwargs.with_schema(bad)


class Test_parse_kwargs_with_resources:
    def test_fail_for_unsupported_cluster_type(self):
        @parse_kwargs.with_resource(Resource.CLUSTER, DbaasOperation.INFO)
        def tgt(**_):
            pass

        with pytest.raises(RuntimeError):
            tgt()


# positive cases checked in func tests
