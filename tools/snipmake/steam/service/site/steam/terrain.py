# -*- coding: utf-8 -*-

import os

from django.contrib.sites.models import Site
from django_nose import NoseTestSuiteRunner
from lettuce import before, world, after
from selenium import webdriver

from core.models import User
from core.features.my_cookies import SESSION_ID
from core.settings import APP_ROOT

from selenium.common.exceptions import NoSuchElementException


CREDENTIALS = {
    'developer': ('robot-steam-dv', 'KSfl2OSfm5'),
    'assessor': ('robot-steam-as', 'KSf2lP1ng'),
    'aadmin': ('robot-steam-aa', 'JSf2poMdfg')
}

@before.all
def before_all():
    world.test_runner = NoseTestSuiteRunner()


@before.each_feature
def set_browser(feature):
    feature_name = os.path.basename(feature.described_at.file).split('.')[0]
    world.data_dir = os.path.join(APP_ROOT, 'features/test_data/')
    world.base_url = 'https://steam-test.yandex-team.ru/'
    world.verificationErrors = []
    world.accept_next_alert = True
    if feature_name in CREDENTIALS:
        profile = webdriver.FirefoxProfile()
        profile.set_preference('browser.download.folderList', 2) # custom location
        profile.set_preference('browser.download.manager.showWhenStarting', False)
        profile.set_preference('browser.download.dir', world.data_dir)
        profile.set_preference('browser.helperApps.neverAsk.saveToDisk', 'text/plain')
        world.driver = webdriver.Firefox(firefox_profile=profile)
        login_to_passport(world.driver, feature_name)
    else:
        world.drivers = {}
        for role in CREDENTIALS:
            login = CREDENTIALS[role][0]
            profile = webdriver.FirefoxProfile()
            world.drivers[login] = webdriver.Firefox(firefox_profile=profile)
            login_to_passport(world.drivers[login], role)
    if feature_name == 'developer':
        init_db()


def login_to_passport(driver, role):
    driver.implicitly_wait(10)
    driver.get('https://passport.yandex-team.ru/')
    driver.find_element_by_xpath('//input[@name="login"]').clear()
    driver.find_element_by_xpath(
        '//input[@name="login"]'
    ).send_keys(CREDENTIALS[role][0])
    driver.find_element_by_xpath('//input[@name="passwd"]').clear()
    driver.find_element_by_xpath(
        '//input[@name="passwd"]'
    ).send_keys(CREDENTIALS[role][1])
    driver.find_element_by_xpath('//input[@type="submit"]').click()

def init_db():
    driver = world.driver
    driver.get(world.base_url + "admin/")
    driver.find_element_by_link_text("Query bins").click()
    try:
        driver.find_element_by_id("action-toggle").click()
        driver.find_element_by_xpath("//select[@name='action']/option[@value='delete_selected']").click()
        driver.find_element_by_name("index").click()
        driver.find_element_by_css_selector("input[type=\"submit\"]").click()
    except NoSuchElementException:
        print("No querybins")

    driver.get(world.base_url + "admin/")
    driver.find_element_by_link_text("Snippet pools").click()
    try:
        driver.find_element_by_id("action-toggle").click()
        driver.find_element_by_xpath("//select[@name='action']/option[@value='delete_selected']").click()
        driver.find_element_by_name("index").click()
        driver.find_element_by_css_selector("input[type=\"submit\"]").click()
    except NoSuchElementException:
        print("No snippetpools")


@after.each_feature
def close_firefox(feature):
    feature_name = os.path.basename(feature.described_at.file).split('.')[0]
    if feature_name in CREDENTIALS:
        world.driver.quit()
    else:
        for login in world.drivers:
            world.drivers[login].quit()
