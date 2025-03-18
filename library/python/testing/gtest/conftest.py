import os
import re
import logging

from xml.etree import ElementTree

import pytest

import library.python.strings

import yatest.common


def pytest_collection_modifyitems(session, config, items):
    new_items = []
    for path in _get_gtest_binaries():
        new_items.extend([GTestItem(path, test_id, None, config, session) for test_id in _list_tests(path)])
    items[:] = new_items


class GTestItem(pytest.Item):
    def __init__(self, binary, name, parent=None, config=None, session=None):
        self._binary = yatest.common.binary_path(binary)
        pytest.Item.__init__(self, name, parent=parent, config=config, session=session, nodeid=name)

    def runtest(self):
        gtest_friendly_name = self._nodeid.replace("::", ".")
        xml_filename = yatest.common.get_unique_file_path(
            yatest.common.output_path(),
            _to_suitable_file_name("{}.run.xml".format(gtest_friendly_name.split(".", 1)[-1]))  # test name wo suite
        )
        res = yatest.common.execute([
            self._binary,
            '--gtest_filter={}'.format(gtest_friendly_name),
            '--gtest_output=xml:{}'.format(xml_filename),
        ], check_exit_code=False)

        if res.exit_code not in [0, 1]:
            raise GTestException(
                'Internal Error: calling {executable} '
                'for test {test_id} failed (returncode={returncode}):\n'
                '{output}'.format(
                    executable=self._binary, test_id=gtest_friendly_name,
                    output=res.std_out,
                    returncode=res.exit_code
                )
            )

        results = self._parse_xml(xml_filename)
        for (executed_test_id, failures, skipped) in results:
            if executed_test_id == gtest_friendly_name:
                if failures:
                    raise GTestException("\n".join(failures))
                if skipped:
                    pytest.skip()
                return None

        raise GTestException(
            'Internal Error: could not find test '
            '{test_id} in results:\n{results}'.format(
                test_id=gtest_friendly_name, results='\n'.join(name for (name, failures, skipped) in results)
            )
        )

    def _parse_xml(self, xml_filename):
        root = ElementTree.parse(xml_filename)
        result = []
        for test_suite in root.findall('testsuite'):
            test_suite_name = test_suite.attrib['name']
            for test_case in test_suite.findall('testcase'):
                test_name = test_case.attrib['name']
                failures = []
                failure_elements = test_case.findall('failure')
                for failure_elem in failure_elements:
                    failures.append(failure_elem.text)
                skipped = test_case.attrib['status'] == 'notrun'
                result.append(
                    (test_suite_name + '.' + test_name, failures, skipped))

        return result

    def repr_failure(self, excinfo):
        if isinstance(excinfo.value, GTestException):
            return GTestFailureRepr(excinfo.value)
        return pytest.Item.repr_failure(self, excinfo)


class GTestException(Exception):
    pass


class GTestFailureRepr(object):

    def __init__(self, exc):
        self._exception = exc

    def __len__(self):
        return len(str(self))

    def __str__(self):
        return "[[bad]]{}[[rst]]".format(self._exception)


def _get_gtest_binaries():
    depends = yatest.common.runtime._get_ya_plugin_instance().dep_roots
    for dep in depends:
        if dep != yatest.common.context.project_path:
            build_path = yatest.common.build_path(dep)
            if os.path.exists(build_path) and os.path.isdir(build_path):
                for file_name in os.listdir(build_path):
                    binary = os.path.join(dep, file_name)
                    try:
                        res = yatest.common.execute([yatest.common.build_path(binary), "--help"])
                        if "This program contains tests written using Google Test." in res.std_out:
                            yield binary
                        else:
                            logging.warning("Binary '{}' does not seem to be a  gtest".format(yatest.common.build_path(binary)))
                    except Exception:
                        logging.exception("{} failed to be verified as a gtest binary")


def _list_tests(path):
    res = yatest.common.execute([yatest.common.binary_path(path), '--gtest_list_tests'])

    def strip_comment(x):
        comment_start = x.find('#')
        if comment_start != -1:
            x = x[:comment_start]
        return x

    test_suite = None
    result = []
    for line in res.std_out.splitlines():
        has_indent = line.startswith(' ')
        if not has_indent and '.' in line:
            test_suite = strip_comment(line).strip().replace(".", "")
        elif has_indent:
            test = (test_suite + "::" + strip_comment(line).strip())
            result.append(test)
    return result


def _to_suitable_file_name(name):
    unicode_free_name = ""
    for symbol in library.python.strings.to_unicode(name, 'UTF-8'):
        code = ord(symbol)
        if code < 128:
            unicode_free_name += chr(code)
        else:
            unicode_free_name += str(hex(code))
    return re.sub("[\[,\]\'\:/]", "_", unicode_free_name)
