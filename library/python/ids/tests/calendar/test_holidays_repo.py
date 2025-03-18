# coding: utf-8
from __future__ import unicode_literals

from datetime import date
import unittest

from ids.exceptions import IDSException
from ids.services.calendar.repositories.holidays import CalendarHolidaysRepository

raw_xml = """<?xml version="1.0" encoding="UTF-8"?>
<holidays>
    <get-holidays>
        <days>
            <day date="2010-12-20"
                is-holiday="0"
                day-type="weekday"
                is-transfer="0"
                holiday-name="День котика">
                День котика
            </day>
            <day date="2010-12-22"
                is-holiday="1"
                day-type="weekend"
                is-transfer="1">
                День котика
            </day>
        </days>
    </get-holidays>
</holidays>
"""


class CalendarHolidaysRepoTest(unittest.TestCase):

    def get_repository(self):
        return CalendarHolidaysRepository(storage=None, server='dummy', user_agent='ids-test')

    def test_fetch_days_as_dicts(self):
        repo = self.get_repository()

        days = list(repo.transform_xml_into_dicts(raw_xml=raw_xml))
        self.assertEqual(
            days[0],
            {
                'date': date(2010, 12, 20),
                'is-transfer': False,
                'is-holiday': False,
                'day-type': 'weekday',
                'holiday-name': u"День котика",
            }
        )
        self.assertEqual(
            days[1],
            {
                'date': date(2010, 12, 22),
                'is-transfer': True,
                'is-holiday': True,
                'day-type': 'weekend',
            }
        )

    def test_lookup_required_params_ok(self):
        repo = self.get_repository()

        try:
            repo.handle_lookup_required_params({
                'start_date': '2012-01-01',
                'who_am_i': 'some_service',
            })
        except IDSException as e:
            assert False, 'required params error %s' % e

    def test_lookup_required_params_error(self):
        repo = self.get_repository()

        self.assertRaises(
            IDSException,
            repo.handle_lookup_required_params,
            {}
        )

        self.assertRaises(
            IDSException,
            repo.handle_lookup_required_params,
            {
                'start_date': '2012-01-01',
            }
        )

        self.assertRaises(
            IDSException,
            repo.handle_lookup_required_params,
            {
                'who_am_i': '2012-01-01',
            }
        )

    def test_lookup_wrong_format_params_error(self):
        repo = self.get_repository()

        self.assertRaises(
            IDSException,
            repo.handle_lookup_dates,
            {
                'start_date': '2012-01-01'
            }
        )

    def test_lookup_defaults_base(self):
        repo = self.get_repository()

        lookup = {}
        repo.handle_lookup_defaults(lookup)

        self.assertEqual(
            lookup,
            {
                'out_mode': 'holidays',
                'country_id': 225,
                'for_yandex': 1,
            }
        )

    def test_lookup_defaults_override(self):
        repo = self.get_repository()

        lookup = {
            'out_mode': 'all',
            'country_id': 100,
            'for_yandex': 0,
            'who_am_i': 'me'
        }
        repo.handle_lookup_defaults(lookup)

        self.assertEqual(
            lookup,
            {
                'out_mode': 'all',
                'country_id': 100,
                'for_yandex': 0,
                'who_am_i': 'me'
            }
        )

    def test_lookup_dates_as_strings(self):
        repo = self.get_repository()

        lookup = {
            'start_date': 'some_string',
        }
        repo.handle_lookup_dates(lookup)
        self.assertEqual(
            lookup,
            {
                'start_date': 'some_string',
            }
        )

    def test_lookup_dates_as_strings(self):
        repo = self.get_repository()

        lookup = {
            'start_date': date(2012, 1, 1),
        }
        repo.handle_lookup_dates(lookup)
        self.assertEqual(
            lookup,
            {
                'start_date': '2012-01-01',
            }
        )
