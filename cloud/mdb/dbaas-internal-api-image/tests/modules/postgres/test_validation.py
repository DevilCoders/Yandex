"""
PostgreSQL cluster validation
"""

import pytest

from dbaas_internal_api.core.exceptions import DbaasClientError, DbaasNotImplementedError
from dbaas_internal_api.modules.postgres.types import Database, UserWithPassword, ClusterConfig
from dbaas_internal_api.utils.version import Version
from dbaas_internal_api.modules.postgres.validation import (
    validate_database,
    validate_databases,
    validate_user,
    validate_version_upgrade,
    validate_search_path,
    ValidationError,
)

# pylint: disable=missing-docstring, invalid-name


def mk_user(name, connect_dbs=None):
    return UserWithPassword(
        name=name,
        connect_dbs=connect_dbs or [],
        encrypted_password={},
        conn_limit=None,
        settings={},
        login=True,
        grants=[],
    )


def mk_db(name, owner, extensions=None):
    return Database(
        name=name,
        owner=owner,
        extensions=extensions if extensions else [],
        lc_collate='C',
        lc_ctype='C',
        template='',
    )


def mk_cluster_config(version):
    return ClusterConfig(
        version=Version(major=version),
        pooler_options={},
        db_options={'user_shared_preload_libraries': []},
        autofailover=False,
        backup_schedule={},
        access={},
        perf_diag={},
    )


class Test_validate_database:
    def test_good_db(self):
        validate_database(mk_db('airplanes', 'Aeroflot'), [mk_user('Aeroflot')], mk_cluster_config(13))

    def test_db_with_invalid_name(self):
        with pytest.raises(DbaasClientError):
            validate_database(mk_db('postgres', 'Aeroflot'), [mk_user('Aeroflot')], mk_cluster_config(13))

    def test_db_with_non_existed_owner(self):
        with pytest.raises(DbaasClientError):
            validate_database(mk_db('airplanes', 'Lufthansa'), [mk_user('Aeroflot')], mk_cluster_config(13))

    def test_duplicated_pg_cron(self):
        validate_databases([mk_db('airplanes', 'Lufthansa', ['pg_cron'])])
        with pytest.raises(DbaasClientError):
            validate_databases(
                [mk_db('airplanes', 'Lufthansa', ['pg_cron']), mk_db('postgres', 'Aeroflot', ['pg_cron'])]
            )


class Test_validate_user:
    def test_good_user(self):
        validate_user(mk_user('Aeroflot', ['airplanes']), [mk_db('airplanes', 'Aeroflot')])

    def test_user_with_invalid_name(self):
        with pytest.raises(DbaasClientError):
            validate_user(mk_user('postgres', ['airplanes']), [mk_db('airplanes', 'Aeroflot')])

    def test_user_with_non_existed_db_in_connect_dbs(self):
        with pytest.raises(DbaasClientError):
            validate_user(mk_user('Aeroflot', ['airplanes', 'cars']), [mk_db('airplanes', 'Aeroflot')])


class Test_validate_version_upgrade:
    def test_valid_upgrade(self):
        validate_version_upgrade('10', '11')

    def test_downgrade(self):
        with pytest.raises(DbaasClientError):
            validate_version_upgrade('11', '10')

    def test_downgrade_w_suffix(self):
        with pytest.raises(DbaasClientError):
            validate_version_upgrade('11-1c', '10-1c')

    def test_suffix_mismatch(self):
        with pytest.raises(DbaasNotImplementedError):
            validate_version_upgrade('10-1c', '11')

    def test_no_path(self):
        with pytest.raises(DbaasNotImplementedError):
            validate_version_upgrade('10', '42')


class Test_validate_search_path:
    def test_valid_search_paths(self):
        paths = [
            "ya_channels, \"$user\", public",
            "\"$user\", bs, public",
            "\"$user\", public,production_campaign",
            "ticketsystem,public",
            "ya_channels,\"$user\", public",
            "test",
            "public",
            "prod,public",
            "\"$user\", public",
            "loh,test",
            "dbo, public",
        ]
        for sp in paths:
            validate_search_path(sp)

    def test_invalid_search_paths(self):
        paths = ["\"$user\", public'\narchive_mode ="]
        for sp in paths:
            with pytest.raises(ValidationError):
                validate_search_path(sp)
