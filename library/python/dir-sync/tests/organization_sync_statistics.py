# coding: utf-8

from __future__ import unicode_literals

from django.test import TestCase

from dir_data_sync.logic import OrganizationSyncStatistics
from dir_data_sync.dao import SyncStatistics


class StatTestCase(TestCase):

    def test_it_works_on_no_exception(self):
        self.assertEqual(SyncStatistics.objects.count(), 0)
        with OrganizationSyncStatistics(dir_org_id=1):
            pass

        self.assertEqual(SyncStatistics.objects.count(), 1)
        sync = SyncStatistics.objects.first()
        self.assertEqual(sync.dir_org_id, '1')
        self.assertEqual(sync.last_pull_status, SyncStatistics.PULL_STATUSES.success)
        self.assertEqual(sync.successful_attempts, 1)

    def test_it_works_on_exception(self):
        self.assertEqual(SyncStatistics.objects.count(), 0)

        def raises_value_error():
            with OrganizationSyncStatistics(dir_org_id=1):
                raise ValueError

        self.assertRaises(ValueError, raises_value_error)

        self.assertEqual(SyncStatistics.objects.count(), 1)
        sync = SyncStatistics.objects.first()
        self.assertEqual(sync.dir_org_id, '1')
        self.assertEqual(sync.last_pull_status, SyncStatistics.PULL_STATUSES.failed)
        self.assertEqual(sync.successful_attempts, 0)
