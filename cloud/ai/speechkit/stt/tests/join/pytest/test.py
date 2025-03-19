import logging

import pytest

from testlib import run_join_e2e_test, list_dir

logger = logging.getLogger("JOIN_MARKUPS_TEST")

movie_105_single_bit_files = list_dir('input_data/movie_105_single_bit', logger)
testamentum_30_files = list_dir('input_data/testamentum_30', logger)
sanity_files = list_dir('input_data/sanity', logger)

logger.debug('sanity_files={}'.format(sanity_files))


def test_case_text_duplication():
    return run_join_e2e_test('input_data/input_case_text_duplication.txt', logger)


@pytest.mark.parametrize('markup_id', movie_105_single_bit_files)
def test_case_movie_105_single_bit(markup_id):
    return run_join_e2e_test('input_data/movie_105_single_bit/{markup_id}'.format(markup_id=markup_id), logger)


@pytest.mark.parametrize('markup_id', testamentum_30_files)
def test_case_testamentum_30(markup_id):
    return run_join_e2e_test('input_data/testamentum_30/{markup_id}'.format(markup_id=markup_id), logger)


@pytest.mark.parametrize('markup_id', sanity_files)
def test_case_sanity(markup_id):
    return run_join_e2e_test('input_data/sanity/{markup_id}'.format(markup_id=markup_id), logger)
