# coding: utf-8
from __future__ import unicode_literals

import itertools

from ids.registry import registry
from ids.repositories.base import RepositoryBase

from .. import utils
from ..resource import Absence
from ..connector import GapConnector as Connector


@registry.add_simple
class AbsenceRepository(RepositoryBase):
    """
    Репозиторий для отсутствий сотрудников.

    Kinda usage:
    >>> from ids.registry import registry
    >>> repo = registry.get_repository('gap', 'absences', user_agent='myservice', token='teamcity')
    >>> repo.get({
    ...     'period_from': '2013-01-01',
    ...     'period_to': '2014-01-01',
    ...     'login_or_list': 'desh'
    ... })
    ...
    ... [
    ...     Absence(date_from=datetime.date(2013, 2, 23), ...),
    ...     Absence(date_from=datetime.date(2013, 5, 20), ...),
    ... ]
    >>> repo.filter(
    ...     period_from='2011-01-01',
    ...     period_to='2013-01-01',
    ...     login_or_list=['desh', 'thasonic']
    ... )
    ...
    ... {
    ...     'desh': [
    ...         Absence(date_from=datetime.date(2013, 2, 23), ...),
    ...         Absence(date_from=datetime.date(2013, 5, 20), ...),
    ...     ],
    ...     'thasonic': [...],
    ... }
    """

    SERVICE = 'gap'
    RESOURCES = 'absences'

    def __init__(self, storage=None, token=None, **options):
        super(AbsenceRepository, self).__init__(storage, **options)
        self.connector = Connector(token=token, **options)

    def getiter_from_service(self, lookup):
        """
        Если кто-нибудь вдруг любит несгруппированный список.
        Кажется что-то другое отсюда возвращать нелогично.
        """
        grouped = self.filter(**lookup)
        return itertools.chain.from_iterable(grouped.values())

    def filter(self, period_from, period_to, login_or_list, **request_params):
        """
        Получить отсутствия для сотрудников, сгруппированные по сотрудникам.
        @param period_from: строка в iso или datetime.date
        @param period_to: строка в iso или datetime.date
        @param login_or_list: смотри докстринг к utils.smart_fetch_logins
        @param request_params: передаются в мтоды requests
        @return: dict. {<логин_сотрудника>: [Absence(), ...]}
        """
        url_vars = {
            'period_from': utils.serialize_period(period_from),
            'period_to': utils.serialize_period(period_to),
            'login_list': ','.join(utils.smart_fetch_logins(login_or_list)),
        }

        response = self.connector.get(
            resource='gap_list',
            url_vars=url_vars,
            **request_params
        )
        data = response.json()

        person_absences = {}
        for login, absences_info in data['gap_list'].items():
            person_absences[login] = []

            for absence in absences_info['absences']:
                person_absences[login].append(Absence(
                    date_from=utils.date_from_gap_dt_string(absence['from']),
                    date_to=utils.date_from_gap_dt_string(absence['till']),
                    state=absence['state'],
                    type=absence['type'],
                    id=int(absence['id']),
                    login=absence['staff'],
                ))

        return person_absences
