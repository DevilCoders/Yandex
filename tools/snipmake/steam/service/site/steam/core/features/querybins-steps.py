# -*- coding: utf-8 -*-

import os

from django.utils import timezone
from lettuce import *
from nose.tools import assert_equals
from selenium.webdriver.common.by import By
from selenium.webdriver.support import expected_conditions
from selenium.webdriver.support.ui import WebDriverWait


@step('I upload file "(.*?)"')
def upload_file(step, filename):
    world.driver.find_element_by_id('id_file').send_keys(
        os.path.join(world.data_dir, filename)
    )
    WebDriverWait(world.driver, 10).until(
        expected_conditions.visibility_of_element_located(
            (By.ID, 'id_file_delete_link')
        )
    )


@step('I see QueryBin "(.*?)"')
def get_querybin(step, title):
    assert_equals(world.driver.find_element_by_xpath('//tr[1]/td[1]').text,
                  title)


@step('I see the window with "(.*?)" header')
def get_window_header(step, header):
    assert_equals(header,
                  world.driver.find_element_by_css_selector('h3').text)


@step('I get back the table with the following data:')
def get_table(step):
    keys = [world.driver.find_element_by_xpath('//th[%d]' % (i + 1)).text
            for i in range(len(step.hashes[0]))]
    for hash_num, hash_obj in enumerate(step.hashes):
        for header_num, header in enumerate(keys):
            assert_equals(world.driver.find_element_by_xpath('//tr[%d]/td[%d]' % (hash_num + 1, header_num + 1)).text,
                          hash_obj[header])


@step('I download first file')
def download_first_file(step):
    dr = world.driver
    dl_url = dr.find_element_by_xpath('//tr[1]/td[1]/a').get_attribute('href')
    world.qb_file = os.path.join(world.data_dir, dr.find_element_by_xpath('//tr[1]/td[1]/a').text)
    dr.get(dl_url)


@step('downloaded file is equal to "(.*?)"')
def check_file(step, file2):
    f1 = open(world.qb_file)
    f2 = open(os.path.join(world.data_dir, file2))
    content1 = f1.read()
    content2 = f2.read()
    f1.close()
    f2.close()
    os.remove(world.qb_file)
    assert_equals(content1, content2)
