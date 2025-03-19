# coding: utf-8
"""
infra utils test
"""
from typing import List

import pytest

from dbaas_internal_api.utils.infra import __name__ as PATH
from dbaas_internal_api.utils.infra import suggest_similar_flavor, suggest_similar_flavor_from


def _mk_fl(name, cpu_guarantee, fl_type='standart'):
    return dict(
        name=name,
        cpu_guarantee=cpu_guarantee,
        memory_guarantee=cpu_guarantee * 1000,
        type=fl_type,
        generation=100,
    )


def _mk_valid_resources(*valid_fl_names):
    return [dict(id=fl_id) for fl_id in valid_fl_names]


# pylint: disable=invalid-name
# pylint: disable=missing-docstring


class Test_suggest_similar_flavor_from:
    def test_raise_when_for_flavor_exists_in_flavors(self):
        with pytest.raises(RuntimeError):
            suggest_similar_flavor_from(
                [
                    _mk_fl('nano', 0.5),
                    _mk_fl('small', 1),
                    _mk_fl('large', 2),
                ],
                _mk_fl('nano', 0.5),
            )

    def test_empty_flavors_is_ok(self):
        assert suggest_similar_flavor_from([], _mk_fl('legacy', 42)) is None

    def test_when_exists_with_same_guarantee(self):
        assert (
            suggest_similar_flavor_from(
                [
                    _mk_fl('nano', 0.5),
                    _mk_fl('small', 1),
                    _mk_fl('large', 2),
                ],
                _mk_fl('legacy', 1),
            )
            == 'small'
        )

    def test_closest_smaller(self):
        assert (
            suggest_similar_flavor_from(
                [
                    _mk_fl('nano', 0.5),
                    _mk_fl('small', 1),
                    _mk_fl('large', 2),
                ],
                _mk_fl('legacy', 0.3),
            )
            == 'nano'
        )

    def test_closest_bigger(self):
        assert (
            suggest_similar_flavor_from(
                [
                    _mk_fl('nano', 0.5),
                    _mk_fl('small', 1),
                    _mk_fl('large', 2),
                ],
                _mk_fl('legacy', 1.7),
            )
            == 'large'
        )


Rows = List[dict]


class Test_suggest_similar_flavor:
    def setup_mocker(
        self, mocker, get_flavors: Rows, get_valid_resources: Rows, get_decommissioning_flavors: List[str]
    ):
        mocker.patch(f'{PATH}.metadb.get_flavors').return_value = get_flavors
        mocker.patch(f'{PATH}.metadb.get_valid_resources').return_value = get_valid_resources
        mocker.patch(f'{PATH}.config.get_decommissioning_flavors').return_value = get_decommissioning_flavors

    def test_when_exists_flavor_with_same_type(self, mocker):
        self.setup_mocker(
            mocker,
            [
                _mk_fl('s.small.legacy', 0.9),
                _mk_fl('s.nano', 0.5),
                _mk_fl('s.small', 1),
                _mk_fl('s.large', 2),
            ],
            _mk_valid_resources('s.small.legacy', 's.nano', 's.small', 's.large'),
            [
                's.small.legacy',
            ],
        )
        assert suggest_similar_flavor('s.small.legacy', 'test_cluster', 'test_roles') == 's.small'

    def test_when_not_exists_flavor_with_same_type(self, mocker):
        self.setup_mocker(
            mocker,
            [
                _mk_fl('s.small.legacy', 0.9, 'standart'),
                _mk_fl('m.nano', 0.5, 'memory'),
                _mk_fl('m.small', 1, 'memory'),
                _mk_fl('m.large', 2, 'memory'),
            ],
            _mk_valid_resources('s.small.legacy', 'm.nano', 'm.small', 'm.large'),
            [
                's.small.legacy',
            ],
        )
        assert suggest_similar_flavor('s.small.legacy', 'test_cluster', 'test_roles') == 'm.small'

    def test_raise_if_no_flavors_found(self, mocker):
        self.setup_mocker(
            mocker,
            [
                _mk_fl('s.small.legacy', 0.9, 'standart'),
                _mk_fl('s.large.legacy', 0.5, 'memory'),
            ],
            _mk_valid_resources('s.small.legacy', 's.large.legacy'),
            ['s.small.legacy', 's.large.legacy'],
        )
        with pytest.raises(RuntimeError):
            suggest_similar_flavor('s.small.legacy', 'test_cluster', 'test_roles')

    def test_raise_if_legacy_flavor_not_found_in_flavors(self, mocker):
        self.setup_mocker(
            mocker,
            [
                _mk_fl('s.small.legacy', 0.9),
                _mk_fl('s.nano', 0.5),
                _mk_fl('s.small', 1),
                _mk_fl('s.large', 2),
            ],
            _mk_valid_resources('s.small.legacy', 's.nano', 's.small', 's.large'),
            [
                's.small.legacy',
            ],
        )
        with pytest.raises(RuntimeError):
            suggest_similar_flavor('SUPERLEGACY', 'test_cluster', 'test_roles')
