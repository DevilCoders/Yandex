# -*- coding: utf-8 -*-
from lettuce import *
from nose.tools import assert_equals

world.switch_labels = {'English': u'Русский (ru)', u'Русский': 'English (en)'}
world.texts = {
    'English': 'Requests',
    'Russian': u'Запросы'
}

@step(u'My current language is "(.*?)":')
def current_language(step, language):
    world.driver.get(world.base_url)
    assert world.driver.find_element_by_xpath(
        '//button[@type="button"]'
    ).text.startswith(language)
    world.tested_language = language


@step('I switch language')
def switch_language(step):
    world.driver.find_element_by_xpath('//button[@type="button"]').click()
    world.driver.find_element_by_link_text(
        world.switch_labels[world.tested_language]
    ).click()


@step('I see (.*?) text')
def check_text(step, language):
    assert_equals(world.texts[language],
                  world.driver.find_element_by_xpath('//a[@class="dropdown-toggle"][1]').text)
