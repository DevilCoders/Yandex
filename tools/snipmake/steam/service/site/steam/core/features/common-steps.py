from lettuce import *
from nose.tools import assert_equals


@step('I submit the form')
def submit_form(step):
    world.driver.find_element_by_xpath('//input[@type="submit"]').click()


@step('I am on the "(.*?)" page')
def set_page(step, url):
    world.driver.get(''.join((world.base_url, url)))


@step('I see "(.*?)" header')
def see_page_header(step, header):
    assert_equals(header, world.driver.find_element_by_tag_name('h1').text)


@step('I see "(.*?)" paragraph')
def see_text(step, header):
    assert_equals(header, world.driver.find_element_by_tag_name('p').text)


@step('I see "(.*?)" link')
def see_link(step, link_name):
    world.driver.find_element_by_link_text(link_name)


@step('I click "(.*?)" button')
def click_button(step, label):
    world.driver.find_element_by_link_text(label).click()


@step('I specify the title "(.*?)"')
def specify_title(step, title):
    world.driver.find_element_by_id('id_title').clear()
    world.driver.find_element_by_id('id_title').send_keys(title)


@step('I complete current task')
def complete_task(step):
    dr = world.driver
    dr.find_element_by_xpath('//input[@class="hotkeys_81"]').click()
    dr.find_element_by_id('submit').click()
    dr.find_element_by_xpath('//input[@class="hotkeys_87"]').click()
    dr.find_element_by_id('submit').click()
    dr.find_element_by_xpath('//input[@class="hotkeys_69"]').click()
    dr.find_element_by_id('id_comment').send_keys('test')
    dr.find_element_by_id('submit').click()


@step('I delete first element')
def delete_first_element(step):
    dr = world.driver
    dr.find_element_by_link_text('Delete').click()
    dr.find_element_by_link_text('Yes').click()
