from dataclasses import dataclass
from typing import List

from .errors import Error


@dataclass
class TestCase:
    name: str
    type: str
    size: int  # GB
    verify_test_cmd_args: str


def generate_test_cases(test_suite: str, service: str) -> List[TestCase]:
    if service == 'nbs':
        types = ['network-ssd', 'network-ssd-v2']
    else:
        types = ['network-ssd']
    test_suite_to_func = {
        '512bytes-bs': _generate_test_cases_for_512bytes_bs_test_suite,
        '4kb-bs': _generate_test_cases_for_4kb_bs_test_suite,
        '64MB-bs': _generate_test_cases_for_64mb_bs_test_suite,
        'ranges-intersection': _generate_test_cases_for_ranges_intersection_test_suite,
    }
    try:
        func = test_suite_to_func[test_suite]
    except KeyError:
        raise Error(f'no such test suite <{test_suite}>')
    return func(types)


def _generate_test_cases_for_512bytes_bs_test_suite(types: [str]) -> List[TestCase]:
    return [
        TestCase(
            name=f'512bytes-bs-{type}',
            type=type,
            size=320,
            verify_test_cmd_args=f'--blocksize=512 --iodepth=64 --filesize={512 * 1024 ** 2}'
        )
        for type in types
    ]


def _generate_test_cases_for_4kb_bs_test_suite(types: [str]) -> List[TestCase]:
    return [
        TestCase(
            name=f'4K-bs-{type}',
            type=type,
            size=320,
            verify_test_cmd_args=f'--blocksize=4096 --iodepth=64 --filesize={64 * 1024 ** 2}'
        )
        for type in types
    ]


def _generate_test_cases_for_64mb_bs_test_suite(types: [str]) -> List[TestCase]:
    return [
        TestCase(
            name=f'64MB-bs-{type}',
            type=type,
            size=320,
            verify_test_cmd_args=f'--blocksize={64 * 1024 ** 2} --iodepth=64 --filesize={16 * 1024 ** 3}'
        )
        for type in types
    ]


def _generate_test_cases_for_ranges_intersection_test_suite(types: [str]) -> List[TestCase]:
    return [
        TestCase(
            name=f'ranges-intersection-{type}',
            type=type,
            size=1024,
            verify_test_cmd_args=f'--blocksize={1024 ** 2} --iodepth=64 --offset={1024 * 17} --step=1024'
                                 f' --filesize={1023 * 1024 ** 2}'
        )
        for type in types
    ]
