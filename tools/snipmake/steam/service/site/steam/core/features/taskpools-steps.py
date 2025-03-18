# -*- coding: utf-8 -*-

from lettuce import *
from nose.tools import assert_equals
from selenium.webdriver.support import expected_conditions
from selenium.webdriver.support.ui import WebDriverWait


@step('I create TaskPool "(.*?)"')
def create_pool(step, title):
    step.behave_as('''
       When I click "Add new" button
       When I specify the title "%s"
       When I specify the kind "SideBySide"
       When I specify country "Russia"
       When I specify the priority "1"
       When I specify first SnippetPool "serp"
       When I specify second SnippetPool "serp_with_al"
       When I specify the overlap "1"
       When I specify the pack size "10"
       When I submit the form
    ''' % title)


@step('I see TaskPool "(.*?)"')
def check_pool(step, title):
    WebDriverWait(world.driver, 30).until(
        expected_conditions.alert_is_present()
    )
    alert = world.driver.switch_to_alert()
    alert.dismiss()
    assert_equals(world.driver.find_element_by_xpath('//tr[1]/td[2]').text,
                  title)


@step('I specify the kind "(.*?)"')
def specify_kind(step, kind):
    world.driver.find_element_by_xpath('(//button[@type="button"])[4]').click()
    world.driver.find_element_by_link_text(kind).click()


@step('I specify country "(.*?)"')
def specify_country(step, country):
    world.driver.find_element_by_xpath('(//button[@type="button"])[5]').click()
    world.driver.find_element_by_link_text(country).click()


@step('I specify first SnippetPool "(.*?)"')
def specify_first_sp(step, title):
    world.driver.find_element_by_xpath('(//button[@type="button"])[6]').click()
    world.driver.find_element_by_link_text(title).click()


@step('I specify second SnippetPool "(.*?)"')
def specify_second_sp(step, title):
    world.driver.find_element_by_xpath('(//button[@type="button"])[7]').click()
    world.driver.find_element_by_link_text(title).click()


@step('I specify the priority "(.*?)"')
def specify_priority(step, priority):
    world.driver.find_element_by_id('id_priority').clear()
    world.driver.find_element_by_id('id_priority').send_keys(priority)


@step('I specify the overlap "(.*?)"')
def specify_overlap(step, overlap):
    world.driver.find_element_by_id('id_overlap').clear()
    world.driver.find_element_by_id('id_overlap').send_keys(overlap)


@step('I specify the pack size "(.*?)"')
def specify_pack_size(step, pack_size):
    world.driver.find_element_by_id('id_pack_size').clear()
    world.driver.find_element_by_id('id_pack_size').send_keys(pack_size)


@step('I click play button')
def click_play(step):
    world.driver.find_element_by_xpath('//tr[1]/td[1]/a').click()


@step('I see stop button')
def check_stop(step):
    assert_equals('icon-stop',
                  world.driver.find_element_by_xpath(
                      '//tr[1]/td[1]/div/a/i'
                  ).get_attribute('class'))
