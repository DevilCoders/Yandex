import logging
import importlib
import pkgutil


def _list_tests(module):
    all_test_cases = [
        importlib.import_module("{}.{}".format(module.__name__, test_case_module_info[1]))
        for test_case_module_info in filter(
            lambda x: x[1].startswith("test_"),
            list(pkgutil.walk_packages(module.__path__)),
        )
    ]
    return all_test_cases


def run_all_tests(module, site_checker):
    all_test_cases = _list_tests(module)

    for test_case in all_test_cases:
        logging.info("RUNNING %s", test_case.__name__)
        logging.info("=" * 120)
        test_case.run_test(site_checker)
        logging.info("=" * 120)

    logging.info("%s test cases OK", len(all_test_cases))
