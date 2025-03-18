# -*- coding: utf-8 -*-

from lettuce import *
from nose.tools import assert_equal
import json
import re
import random
from threading import Thread

from core.models import Estimation


EST_ID_RE = re.compile(r'Estimation (\d+)')


class Aadmin:
    def __init__(self, login):
        self.base_url = world.base_url
        self.login = login
        self.driver = world.drivers[login]

    def bind_tasks_to_ests(self, tab):
        self.driver.get('%susertasks/%s/' % (self.base_url, tab))
        est_ids = [int(EST_ID_RE.search(elem.text).group(1))
                   for elem in self.driver.find_elements_by_xpath(
                       '//table/tbody/tr/td/div/div/a'
                   )]
        self.tasks_to_ests = {}
        for est_id in est_ids:
            self.driver.get(
                '%sadmin/core/estimation/%d/' % (self.base_url, est_id)
            )
            task_unicode = self.driver.find_element_by_id(
                'id_task'
            ).get_attribute('value')
            self.tasks_to_ests[int(task_unicode.split('-')[0])] = est_id

    def take_ests(self, task_ids):
        self.driver.get(''.join((self.base_url, 'taskpools/')))
        self.driver.find_element_by_xpath('//tr[1]/td[2]/a').click()
        for task_id in task_ids:
            self.driver.find_element_by_xpath(
                '//tr/td/label/input[@value="%d"]' % task_id
            ).click()
        self.driver.find_element_by_xpath('//input[@type="submit"]').click()

    def complete_ests(self, value):
        for est_id in self.tasks_to_ests.values():
            self.driver.get('%s/estimation/%d' % (self.base_url, est_id))
            # criterions are estimated in reverse order
            for crit_name in Estimation.Criterion.NAMES[::-1]:
                self.driver.find_element_by_xpath(
                    '//input[@name="%s" and @value="%s"]' %
                    (crit_name, value[crit_name])
                ).click()
                self.driver.find_element_by_id('submit').click()

    def change_est(self, est_id, shuffle_value):
        self.driver.get('%sestimation/check/%d/' % (self.base_url, est_id))
        for crit_name in Estimation.Criterion.NAMES:
            cur_val = int(self.driver.find_element_by_xpath(
                '//input[@name="%s" and @checked="checked"]' %
                crit_name
            ).get_attribute('value'))
            new_val = cur_val + shuffle_value
            new_val += -3 * int(new_val > 1)
            self.driver.find_element_by_xpath(
                '//input[@name="%s" and @value="%d"]' %
                (crit_name, new_val)
            ).click()
        self.driver.find_element_by_id('submit').click()
        assert_equal(self.driver.find_element_by_xpath('//h1').text,
                     'Estimation check')

    def check_est_values(self, value):
        for est_id in self.tasks_to_ests.values():
            self.driver.get('%sestimation/check/%d/' % (self.base_url, est_id))
            cur_val = {}
            for crit_name in Estimation.Criterion.NAMES:
                cur_val[crit_name] = self.driver.find_element_by_xpath(
                    '//input[@name="%s" and @checked="checked"]' %
                    crit_name
                ).get_attribute('value')
            assert_equal(cur_val, value)


@step('base aadmin is "([^"]*)"')
def set_base_aadmin(step, login):
    world.base_aadmin = Aadmin(login)


@step('additional aadmins are:')
def set_additional_aadmin(step):
    aadmin_logins = [aadmin_dict['login'] for aadmin_dict in step.hashes]
    world.addit_aadmins = [Aadmin(login) for login in aadmin_logins]


@step('all aadmins have same tasks with estimation value ({.*?})')
def set_same_tasks_for_aadmins(step, value):
    value = json.loads(value)
    world.base_aadmin.bind_tasks_to_ests('finished')
    world.task_ids = world.base_aadmin.tasks_to_ests.keys()
    for aadmin in world.addit_aadmins:
        aadmin.take_ests(world.task_ids)
        aadmin.bind_tasks_to_ests('current')
        aadmin.complete_ests(value)


@step('all aadmins recheck \(shuffle digits on (\d+)\) first (\d+) tasks parallel for (\d+) times')
def parallel_recheck_tasks(step, shuffle_val, tasks_count, iter_count):
    def recheck(aadmin, est_ids, shuffle_val, iter_count, errors, idx):
        print('start "%s"' % aadmin.login)
        try:
            for i in range(iter_count):
                for est_id in est_ids:
                    aadmin.change_est(est_id, shuffle_val)
        except AssertionError:
            errors[idx] = aadmin.login
            raise
        print('stop "%s"' % aadmin.login)

    shuffle_val, tasks_count, iter_count = map(
        int, (shuffle_val, tasks_count, iter_count)
    )

    aadmins = [world.base_aadmin] + world.addit_aadmins
    aadmin_count = len(aadmins)
    threads = [None] * aadmin_count
    errors = [None] * aadmin_count
    task_ids = world.task_ids[:tasks_count]
    for i in range(aadmin_count):
        random.shuffle(task_ids)
        est_ids = [aadmins[i].tasks_to_ests[task_id] for task_id in task_ids]
        threads[i] = Thread(target=recheck, args=(
            aadmins[i], est_ids, shuffle_val, iter_count, errors, i))
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    assert_equal(filter(None, errors), [])


@step('Then all aadmins estimation with value ({.*?})')
def check_estimation_changes(step, value):
    value = json.loads(value)
    for aadmin in [world.base_aadmin] + world.addit_aadmins:
        aadmin.check_est_values(value)
