"""
Postgresql api.convertors tests
"""

from hamcrest import assert_that, contains_inanyorder

from dbaas_internal_api.modules.postgres.api.convertors import __name__ as PACKAGE
from dbaas_internal_api.modules.postgres.api.convertors import database_from_spec, user_from_spec
from dbaas_internal_api.modules.postgres.types import Database

# pylint: disable=missing-docstring, invalid-name


class Test_database_from_spec:
    def test_db_wo_extensions(self):
        assert database_from_spec(
            {
                'name': 'airplanes',
                'owner': 'Aeroflot',
                'extensions': [],
                'lc_collate': 'C',
                'lc_ctype': 'C',
            }
        ) == Database(name='airplanes', owner='Aeroflot', extensions=[], lc_collate='C', lc_ctype='C', template='')

    def test_db_with_extensions(self):
        assert database_from_spec(
            {
                'name': 'airplanes',
                'owner': 'JAL',
                'extensions': [
                    {
                        'name': 'postgis',
                    }
                ],
                'lc_collate': 'UTF8.ja',
                'lc_ctype': 'UTF8.ja',
            }
        ) == Database(
            name='airplanes',
            owner='JAL',
            extensions=['postgis'],
            lc_collate='UTF8.ja',
            lc_ctype='UTF8.ja',
            template='',
        )


class Test_user_from_spec:
    dbs = [
        Database('airplanes_db', 'Aeroflot', [], 'UTF8.ru', 'UTF8.ru', ''),
        Database('flights_db', 'DME', [], 'UTF8.ru', 'UTF8.ru', ''),
        Database('cars_db', 'BMW', [], 'UTF8.ru', 'UTF8.ru', ''),
    ]

    encrypted_value = {'version': 42, 'value': 'encrypted'}

    def mock(self, mocker):
        mocker.patch(PACKAGE + '.encrypt', return_value=self.encrypted_value)

    def test_user_from_spec_encrypt_password(self, mocker):
        self.mock(mocker)
        user = user_from_spec(
            {
                'name': 'Aeroflot',
                'password': 'blah',
            },
            self.dbs,
        )
        assert user.encrypted_password == self.encrypted_value

    def test_user_from_spec_wo_connect_dbs(self, mocker):
        self.mock(mocker)
        user = user_from_spec(
            {
                'name': 'Aeroflot',
                'password': 'blah',
            },
            self.dbs,
        )
        assert_that(
            user.connect_dbs,
            contains_inanyorder(
                'airplanes_db',
                'flights_db',
                'cars_db',
            ),
        )

    def test_user_with_permissions(self, mocker):
        self.mock(mocker)
        user = user_from_spec(
            {
                'name': 'Aeroflot',
                'password': 'blah',
                'permissions': [
                    {
                        'database_name': 'flights_db',
                    }
                ],
            },
            self.dbs,
        )
        assert_that(
            user.connect_dbs,
            contains_inanyorder('airplanes_db', 'flights_db'),
        )
