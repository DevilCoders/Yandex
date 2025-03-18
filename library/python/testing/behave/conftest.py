import os
import logging
import re

from xml.etree import ElementTree

import pytest

import behave.__main__ as behave_main
import behave.configuration


import yatest.common
from yatest_lib.ya import Ya


logger = logging.getLogger("behave_runner")


def pytest_collection_modifyitems(session, config, items):
    yatest.common.runtime._set_ya_config(ya=Ya())
    tests = []
    for t in _get_test_list():
        tests.append(BehaveItem.from_parent(feature=t[0], suite_name=t[1], test_name=t[2], parent=session))
    items[:] = tests


class BehaveItem(pytest.Item):
    def __init__(self, feature, suite_name, test_name, **kwargs):
        self._feature = feature
        self._test_name = test_name
        self._site_name = suite_name
        pytest.Item.__init__(self, feature, nodeid="::".join([suite_name, test_name]), **kwargs)

    @classmethod
    def from_parent(cls, **kwargs):
        behave_item = getattr(super(BehaveItem, cls), 'from_parent', cls)(**kwargs)
        return behave_item

    def runtest(self):
        if self.config.option.test_filter:
            results = _run_behave(include_re="features.{}".format(self._feature), name=["{}$".format(self._test_name)])
        elif hasattr(self.config, "_behave_results"):
            results = self.config._behave_results
        else:
            results = self.config._behave_results = _run_behave()
        for (feature, suite_name, test_name, system_out, failures, skipped) in results:
            if self._feature == feature and test_name == self._test_name and self._site_name == suite_name:
                if system_out:
                    logger.info("\n".join(system_out))
                if failures:
                    pytest.fail("\n".join(failures), pytrace=False)
                if skipped:
                    pytest.skip()

                return None
        raise BehaveException("Test {} was not found".format(self._test_name))


class BehaveException(Exception):
    pass


def _run_behave(**kwargs):
    config = behave.configuration.Configuration(
        command_args=[],
        junit=True,
        junit_directory=os.path.join(yatest.common.output_path(), "reports"),
        paths=[yatest.common.test_source_path("features")],
        **kwargs
    )
    if not os.path.exists(config.junit_directory):
        os.makedirs(config.junit_directory)

    behave_main.run_behave(config)
    tests = []

    for junit_file_name in os.listdir(config.junit_directory):
        tests += _parse_xml(os.path.join(config.junit_directory, junit_file_name))

    return tests


def _get_test_list():
    return _run_behave(dry_run=True)


def _get_failures_string(test_case):
    for failure_elem in test_case.findall('failure') + test_case.findall('error'):
        find_fail = False
        if failure_elem.get("message") and failure_elem.get("message") != "None":
            find_fail = True
            yield failure_elem.get("message")

        if failure_elem.text:
            find_fail = True
            yield failure_elem.text

        if not find_fail:
            yield ElementTree.tostring(failure_elem, encoding="utf-8", method="text")


def _parse_xml(xml_filename):
    feature_name = re.search("TESTS-features.(.*?).xml", os.path.basename(xml_filename)).groups()[0]
    root = ElementTree.parse(xml_filename)
    result = []
    test_suite = root.getroot()
    test_suite_name = test_suite.attrib['name']
    for test_case in test_suite.findall('testcase'):
        test_name = test_case.attrib['name']
        failures = list(_get_failures_string(test_case))

        system_out = []
        system_out_elements = test_case.findall('system-out')
        for system_out_element in system_out_elements:
            system_out.append(system_out_element.text)
        skipped = test_case.attrib['status'] == 'notrun'
        if test_case.attrib['status'] == 'untested' and not failures:
            failures = [
                'It looks like this scenario fell in `before_all` hook.',
                'Examine the test stdout.',
            ]

        result.append(
            (feature_name, test_suite_name, test_name, system_out, failures, skipped))
    return result
