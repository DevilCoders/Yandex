from typing import List

from pydantic import BaseModel

from database.models import UploadProberLogPolicy
from tools.synchronize.equalable import EqualableMixin


def test_convert_lists_to_sets_works_with_dicts():
    result = EqualableMixin.convert_lists_to_sets(
        [
            {"a": 1, "b": 2},
            {"a": 100, "b": 200}
        ]
    )
    assert len(result) == 2
    assert isinstance(result, frozenset)
    for elem in result:
        assert "a" in elem
        assert "b" in elem
        assert elem["b"] == elem["a"] * 2


def test_convert_lists_to_sets_not_fail_on_complex_structure():
    EqualableMixin.convert_lists_to_sets(
        [{
            "id": None,
            "is_prober_enabled": True,
            "interval_seconds": 10,
            "timeout_seconds": 60,
            "s3_logs_policy": UploadProberLogPolicy.FAIL,
            "cluster": {
                "id": None,
                "arcadia_path": "clusters/prod/meeseeks/cluster.yaml",
                "name": "Meeseeks",
                "slug": "meeseeks",
                "recipe": {
                    "id": None,
                    "arcadia_path": "recipes/meeeseeks/recipe.yaml",
                    "name": "meeseeks",
                    "description": "Small independent virtual machines on each compute node",
                    "files": [
                        {
                            "id": None,
                            "is_executable": True,
                            "relative_file_path": "conductor.tf",
                            "content": b"bla-bla-bla",
                        },
                        {
                            "id": None,
                            "is_executable": False,
                            "relative_file_path": "instances.tf",
                            "content": b"bla-bla-bla-2"
                        }
                    ]
                },
                "variables": {
                    "cluster_id": 1,
                },
            },
            "hosts_re": None,
        }]
    )


def test_equable_mixin():
    class Cls(BaseModel, EqualableMixin):
        str_field: str
        int_field: int

    first = Cls(str_field="value 1", int_field=1)
    second = Cls(str_field="value 2", int_field=1)

    assert not first.equals(second)


def test_equable_mixin_with_ignored_keys():
    class Cls(BaseModel, EqualableMixin):
        str_field: str
        int_field: int

        class Config:
            comparator_ignores_keys = {"str_field"}

    first = Cls(str_field="value 1", int_field=1)
    second = Cls(str_field="value 2", int_field=1)

    assert first.equals(second)


def test_equable_mixin_with_complex_structure():
    class ChildCls(BaseModel, EqualableMixin):
        id: int
        arcadia_path: str
        name: str

        class Config:
            comparator_ignores_keys = {"id"}

    class Cls(BaseModel, EqualableMixin):
        child: List[ChildCls]
        field: str

        class Config:
            comparator_ignores_keys = {"child": {"__all__": ChildCls.Config.comparator_ignores_keys}}

    first = Cls(child=[ChildCls(id=10, arcadia_path="...", name="test")], field="value")
    second = Cls(child=[ChildCls(id=20, arcadia_path="...", name="test")], field="value")

    assert first.equals(second)

    second.child[0].arcadia_path = "updated"

    assert not first.equals(second)


def test_equable_mixin_equal_sets():
    class Cls(BaseModel, EqualableMixin):
        str_field: str
        int_field: int

        class Config:
            comparator_ignores_keys = {"str_field"}

    first_set = [Cls(str_field="a", int_field=1), Cls(str_field="b", int_field=2)]
    second_set = [Cls(str_field="c", int_field=2), Cls(str_field="d", int_field=1)]

    assert EqualableMixin.equal_sets(first_set, second_set)
