import os

import xml.dom.minidom as xml_parser

import test.const


def parse_junit(junit_path,  classname_prefix=None):
    with open(junit_path) as junit:
        report = xml_parser.parseString(junit.read())

    suites = []
    for suite_el in report.getElementsByTagName("testsuite"):
        suite = {"name": suite_el.attributes["name"].value, "subtests": []}

        for attr in ["failures", "name", "skipped", "tests", "time"]:
            if suite_el.attributes.has_key(attr):  # noqa
                suite[attr] = suite_el.attributes[attr].value

        for test_case_el in suite_el.getElementsByTagName("testcase"):
            test_case_name = test_case_el.attributes["name"].value
            class_name = test_case_el.attributes["classname"].value

            # remove path to the file directory from class_name
            if test_case_el.attributes.has_key("file"):  # noqa
                file_name = test_case_el.attributes["file"].value
                class_name_to_be_stripped = os.path.splitext(file_name)[0].replace(os.sep, ".")
                class_name = class_name.replace(class_name_to_be_stripped, "").strip(".")
                test_case_name = ".".join(filter(None, [class_name, test_case_name])).replace(".", "::")
                class_name = os.path.basename(file_name)

            if test_case_el.getElementsByTagName("failure"):
                test_comment = "[[bad]]{}[[rst]]".format(test_case_el.getElementsByTagName("failure")[0].childNodes[0].wholeText)
                test_status = test.const.Status.FAIL
            elif test_case_el.getElementsByTagName("error"):
                test_comment = "[[bad]]{}[[rst]]".format(test_case_el.getElementsByTagName("error")[0].childNodes[0].wholeText)
                test_status = test.const.Status.FAIL
            elif test_case_el.getElementsByTagName("skipped"):
                test_comment = test_case_el.getElementsByTagName("skipped")[0].attributes["message"].value
                test_status = test.const.Status.SKIPPED
            else:
                test_comment = ""
                test_status = test.const.Status.GOOD

            suite["subtests"].append({
                "class_name": class_name,
                "name": test_case_name,
                "comment": test_comment,
                "status": test.const.Status.TO_STR[test_status],
                "duration": float(test_case_el.attributes["time"].value),
            })

            suites.append(suite)

    return suites
