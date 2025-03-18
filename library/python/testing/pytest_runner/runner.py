import os
import yatest.common
import yatest.common.runtime
import junit
import trace


def import_junit_report(suites, work_dir, trace_reporter, subtest_cb=None):
    for suite in suites:
        for subtest in suite["subtests"]:
            # External callback may change subtest's fields or even return None if it should be ignored
            if subtest_cb is not None:
                subtest = subtest_cb(subtest)
            if not subtest:
                continue
            class_name = subtest["class_name"]
            test_name = subtest["name"]
            suite_error_marker = "environment::arcadia::"
            if suite_error_marker in test_name:
                test_name = test_name[test_name.find(suite_error_marker) + len(suite_error_marker):]
            trace_reporter.on_start_test_class(class_name)
            trace_reporter.on_start_test_case(class_name, test_name, work_dir)
            trace_reporter.on_finish_test_case(class_name, test_name, subtest["status"], subtest["comment"], subtest["duration"], work_dir)
            trace_reporter.on_finish_test_class(class_name)


def import_junit_file(junit_xml_path, work_dir=None, trace_reporter=None, subtest_cb=None):
    if not trace_reporter:
        trace_reporter = trace.TraceReportGenerator(yatest.common.runtime._get_ya_config().option.ya_trace_path)
    if not work_dir:
        work_dir = os.path.dirname(os.path.abspath(junit_xml_path))

    if not os.path.exists(junit_xml_path):
        trace_reporter.on_error("Fail to import %s, file not exits" % junit_xml_path)
        return 0

    suites = junit.parse_junit(junit_xml_path)
    if not suites:
        trace_reporter.on_error("Fail to import %s, test suites not found" % junit_xml_path)
        return 0
    import_junit_report(suites, work_dir, trace_reporter, subtest_cb)
    return 1


def import_junit_files(junit_files, work_dir=None, trace_reporter=None, subtest_cb=None):
    if not trace_reporter:
        trace_reporter = trace.TraceReportGenerator(yatest.common.runtime._get_ya_config().option.ya_trace_path)

    nr_imported = 0
    for fname in junit_files:
        nr_imported += import_junit_file(fname, work_dir, trace_reporter, subtest_cb)
    return nr_imported


def run(test_files, pytest_bin=None, python_path=None, pytest_args=None, work_dir=None, env=None, subtest_cb=None, timeout=None):
    if pytest_bin:
        pytest_cmd = [pytest_bin]
    else:
        if not python_path:
            python_path = yatest.common.python_path()
        pytest_cmd = [python_path, "-B", "-m", "pytest"]

    if not pytest_args:
        pytest_args = []
    result_dir = yatest.common.test_output_path("pytest_run")
    junit_xml_path = os.path.join(result_dir, "junit.xml")
    if not work_dir:
        work_dir = result_dir
    if not os.path.exists(work_dir):
        os.makedirs(work_dir)

    res = yatest.common.execute(
        pytest_cmd + pytest_args + ["--junit-xml", junit_xml_path] + test_files,
        env=env,
        check_exit_code=False,
        timeout=timeout,
        cwd=work_dir
    )
    trace_reporter = trace.TraceReportGenerator(yatest.common.runtime._get_ya_config().option.ya_trace_path)
    if not os.path.exists(junit_xml_path):
        trace_reporter.on_error(res.std_err)
    nr = import_junit_files([junit_xml_path], trace_reporter=trace_reporter, subtest_cb=subtest_cb)
    if not nr:
        trace_reporter.on_error("Test file {} did not produce any test results".format(", ".join(
            ["[[path]]{}[[rst]]".format(test_file) for test_file in test_files]
        )))
