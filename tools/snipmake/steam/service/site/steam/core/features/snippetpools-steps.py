# -*- coding: utf-8 -*-

import os

from django.utils import timezone
from lettuce import *
from nose.tools import assert_equals
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions
from selenium.webdriver.support.ui import WebDriverWait


@step('I upload SnippetPool "(.*?)"')
def upload_pool(step, filename):
    world.driver.find_element_by_id('upload_link').click()
    world.driver.find_element_by_id('id_pool_file').send_keys(
        os.path.join(world.data_dir, filename)
    )
    WebDriverWait(world.driver, 30).until(
        expected_conditions.visibility_of_element_located(
            (By.ID, 'id_pool_file_delete_link')
        )
    )


@step('I see SnippetPool "(.*?)"')
def check_pool(step, title):
    WebDriverWait(world.driver, 300).until(
        expected_conditions.visibility_of_element_located(
            (By.XPATH, '//tr[1]/td[1]/a')
        )
    )
    world.driver.get('/'.join((world.base_url, 'snippetpools')))
    assert_equals(world.driver.find_element_by_xpath('//tr[1]/td[1]').text,
                  title)


@step('I specify the host "(.*?)"')
def specify_host(step, host):
    world.driver.find_element_by_xpath('(//button[@type="button"])[3]').click()
    world.driver.find_element_by_link_text(host).click()


@step('I specify the QueryBin "(.*?)"')
def specify_qb(step, qb):
    world.driver.find_element_by_xpath('(//button[@type="button"])[2]').click()
    world.driver.find_element_by_link_text(qb).click()


@step('I specify CGI-params "(.*?)"')
def specify_cgi_params(step, params):
    world.driver.find_element_by_id('id_cgi_params').clear()
    world.driver.find_element_by_id('id_cgi_params').send_keys(params)

@step('snippet count is equal to "(.*?)"')
def check_snippet_count(step, filename):
    got_snippet_count = int(world.driver.find_element_by_xpath('//tr[1]/td[2]').text)
    from xml.dom import minidom
    xmldoc = minidom.parse(os.path.join(world.data_dir, filename))
    snippet_list = xmldoc.getElementsByTagName('snippet')
    assert_equals(got_snippet_count, len(snippet_list))
