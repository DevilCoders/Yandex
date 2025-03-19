"""Tests for rotate_s3_backups"""
import pprint
from unittest import TestCase
from types import SimpleNamespace

from datetime import timedelta, datetime
from s3_rotate import BackupItem, BackupRotate, load_config
TPL = "test-s3://music/backup/{}/test-base/%F"

def _policy_test_helper(self):
    """Helper for testing policy"""
    policy = self.worker.conf.policy.copy()
    policy.update(self.worker.conf.buckets["music"].policy)
    policy.update(self.worker.conf.buckets["music"]["mysql"].policy)

    valid_backups = []
    tmp = datetime.now()
    for itr in range(1, 400):
        valid_backups.append(BackupItem(
            path=(tmp - timedelta(days=itr - 1)).strftime(TPL.format("mysql"))
        ))
        self.worker.backups_list = valid_backups

        backup_groups = self.worker.group_by_frequency()
        policy = self.worker.conf.policy.copy()
        policy.update(self.worker.conf.buckets["music"].policy)
        policy.update(self.worker.conf.buckets["music"]["mysql"].policy)
        self.worker.select_valid(backup_groups, policy)

        excpect_description = "itr"
        if itr < policy.daily:
            expect = itr
        elif itr < (policy.daily + policy.weekly * 7):
            excpect_description = "daily"
            expect = policy.daily
        elif itr < (policy.daily + policy.weekly * 7 + policy.monthly * 30):
            excpect_description = "daily + weekly"
            expect = policy.daily + policy.weekly
        else:
            excpect_description = "daily + weekly + monthly"
            expect = sum(policy.values())

        preserve_backups = []
        for bkp in sorted(valid_backups, key=lambda x: x.timestamp):
            if bkp.valid and bkp.state == "normal":
                preserve_backups.append(
                    BackupItem(path=bkp.timestamp.strftime(TPL.format("mysql")))
                )
        valid_backups = preserve_backups

        self.assertGreaterEqual(
            len(valid_backups), expect,
            "Itr {}: Too less backups".format(itr))
        self.assertGreaterEqual(
            len(valid_backups), expect,
            msg="Iteration {}: Too less backups: expect({})={}, have={} -> {}".format(
                itr, excpect_description, expect, len(valid_backups),
                pprint.pformat([x.timestamp.strftime("%F") for x in valid_backups])
            )
        )
    return True

class TestSelectValid(TestCase):
    """Tests select preserved backups"""
    args = SimpleNamespace(check="mysql", bucket="music", debug=False)
    worker = BackupRotate(load_config(args))
    def test_select_valid(self):
        """Tests select preserved backups"""
        backups_list = []
        tmp = datetime.now()
        for delta in range(393):
            path = (tmp + timedelta(days=delta)).strftime(TPL.format("mysql"))
            backups_list.append(BackupItem(path=path))
        self.assertTrue(len(backups_list) == 393)

        self.worker.conf.save_from_last = True
        self.worker.backups_list = backups_list
        backup_groups = self.worker.group_by_frequency()
        policy = self.worker.conf.policy.copy()
        policy.update(self.worker.conf.buckets["music"].policy)
        policy.update(self.worker.conf.buckets["music"]["mysql"].policy)
        self.worker.select_valid(backup_groups, policy)

        valid_backups = []
        for bkp in backups_list:
            if bkp.valid and bkp.state == "normal":
                valid_backups.append(bkp)

        expect_by_policy = sum(policy.values())
        self.assertGreaterEqual(
            len(valid_backups), expect_by_policy,
            msg="Backups expect {} != actual {}: {}".format(
                expect_by_policy, len(valid_backups),
                pprint.pformat([x.timestamp.strftime("%F") for x in valid_backups])
            )
        )

    def test_policy_d1w1m1(self):
        """Test d1w1m1 policy"""
        self.worker.conf.policy.update({"daily": 1, "weekly": 1, "monthly": 1})
        self.assertTrue(_policy_test_helper(self))


    def test_policy_d3w1m1(self):
        """Test d3w1m1 policy"""
        self.worker.conf.policy.update({"daily": 3, "weekly": 1, "monthly": 1})
        self.assertTrue(_policy_test_helper(self))


    def test_policy_d1w4m1(self):
        """Test d1w4m1 policy"""
        self.worker.conf.policy.update({"daily": 1, "weekly": 4, "monthly": 1})
        self.assertTrue(_policy_test_helper(self))


    def test_policy_d5w3m2(self):
        """Test d5w3m2 policy"""
        self.worker.conf.policy.update({"daily": 5, "weekly": 3, "monthly": 2})
        self.assertTrue(_policy_test_helper(self))

    def test_policy_d9w5m2(self):
        """Test d9w5m2 policy"""
        self.worker.conf.policy.update({"daily": 9, "weekly": 5, "monthly": 2})
        self.assertTrue(_policy_test_helper(self))

    def test_policy_d2w5m9(self):
        """Test d9w5m9 policy"""
        self.worker.conf.policy.update({"daily": 9, "weekly": 5, "monthly": 9})
        self.assertTrue(_policy_test_helper(self))

    def test_policy_d0w1m1(self):
        """Test d0w1m1 policy"""
        self.worker.conf.policy.update({"daily": 0, "weekly": 1, "monthly": 1})
        self.assertTrue(_policy_test_helper(self))

    def test_policy_d1w0m1(self):
        """Test d1w0m1 policy"""
        self.worker.conf.policy.update({"daily": 1, "weekly": 0, "monthly": 1})
        self.assertTrue(_policy_test_helper(self))

    def test_policy_d1w1m0(self):
        """Test d1w1m0 policy"""
        self.worker.conf.policy.update({"daily": 1, "weekly": 1, "monthly": 0})
        self.assertTrue(_policy_test_helper(self))

    def test_policy_d12w1m9(self):
        """Test d12w1m9 policy"""
        self.worker.conf.policy.update({"daily": 12, "weekly": 1, "monthly": 9})
        self.assertTrue(_policy_test_helper(self))

    def test_policy_d1w12m9(self):
        """Test d1w12m9 policy"""
        self.worker.conf.policy.update({"daily": 1, "weekly": 12, "monthly": 9})
        self.assertTrue(_policy_test_helper(self))
