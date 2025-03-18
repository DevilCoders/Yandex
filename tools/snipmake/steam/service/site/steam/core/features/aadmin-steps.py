# -*- coding: utf-8 -*-

from lettuce import *
from nose.tools import assert_less

from core.actions import taskdealer


@step('I go to the first td page')
def go_to_first_td_page(step):
    dr = world.driver
    taskpool_url = dr.find_element_by_xpath('//tr[1]/td[2]/a').get_attribute('href')
    dr.get(taskpool_url)

@step('I see that assessor is underchecked less than (\d+) tasks')
def check_underchecked_tasks_count(step, threshold):
    threshold = int(threshold)
    dr = world.driver
    total_tasks = int(dr.find_element_by_xpath('//div[4]/div[2]').text)
    complete_tasks = sum([1 for i in range(total_tasks) if dr.find_element_by_xpath('//tr[%d]/td[4]' % (i + 1)).text == 'Complete'])
    insp_tasks = sum([1 for i in range(total_tasks) if dr.find_element_by_xpath('//tr[%d]/td[3]' % (i + 1)).text != 'Regular'])
    assert_less(taskdealer.get_t_testing(complete_tasks, insp_tasks), threshold)

@step('Taskpool is finished')
def finish_taskpool(step):
    driver = world.driver
    driver.get(world.base_url + 'admin/')
    driver.find_element_by_link_text('Task pools').click()
    driver.find_element_by_xpath('//table[@id="result_list"]/tbody/tr[1]/th/a').click()
    driver.find_element_by_xpath('//select[@name="status"]/option[@value="F"]').click()
    driver.find_element_by_name('_save').click()

@step('I take and check available tasks')
def take_and_check_available_tasks(step):
    dr = world.driver
    step.when('I am on the "usertasks/available/" page')
    dr.find_element_by_id('task_controller').click()
    dr.find_element_by_css_selector("div > input.btn").click()
    step.when('I am on the "usertasks/current/" page')
    dr.find_element_by_xpath('//tr[1]/td[1]/div/div/a').click()
    available_url = ''.join((world.base_url, 'usertasks/available/'))
    while dr.current_url != available_url:
        step.when('I complete current task')
        dr.find_element_by_id("next").click()
