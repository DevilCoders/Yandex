# -*- coding: utf-8 -*-

from lettuce import *
from nose.tools import assert_less_equal, assert_equals


@step('I see a pack on "(.*?)" page')
def take_available_pack(step, page):
    step.behave_as('I am on the "%s" page' % page)
    assert_equals(['task', 'pack'],
                  world.driver.find_element_by_xpath('//tr[1]/td[1]').text.split()[1:3])


@step('I complete available tasks')
def complete_available_tasks(step):
    world.driver.find_element_by_xpath('//tr[1]/td[1]/a').click()
    world.driver.find_element_by_xpath('//div/table/tbody/tr[2]/td[1]/div/div/a').click()
    while world.driver.find_element_by_xpath('//h1').text != 'User tasks':
        step.behave_as('I complete current task')
