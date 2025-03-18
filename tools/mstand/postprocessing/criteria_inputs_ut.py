import os

import numpy
import pytest

import postprocessing.criteria_inputs as criteria_inputs
from postprocessing.criteria_inputs import CriteriaReadException
from postprocessing.criteria_read_mode import CriteriaReadMode


def sample_path(data_path, file_name):
    return os.path.join(data_path, "compute_criteria", file_name)


def check_load(file_name, read_mode, data_path, length=None):
    file_path = sample_path(data_path, file_name)
    data = criteria_inputs.load_simple_tsv_lines(file_path, read_mode)
    if length is not None:
        assert len(data) == length
    if read_mode == CriteriaReadMode.FLOAT_1D:
        assert len(data.shape) == 1
    elif read_mode == CriteriaReadMode.FLOAT_2D:
        assert len(data.shape) == 2
        numpy.concatenate(data)
    elif read_mode == CriteriaReadMode.JSON_LISTS:
        numpy.concatenate(data)
    else:
        assert False


# noinspection PyClassHasNoInit
class TestCriteriaInputs:
    def test_load_1d_ok(self, data_path):
        check_load("sample_1d_ok.tsv", CriteriaReadMode.FLOAT_1D, data_path, length=10)

    def test_load_1d_bad(self, data_path):
        with pytest.raises(CriteriaReadException):
            check_load("sample_1d_bad.tsv", CriteriaReadMode.FLOAT_1D, data_path)
        with pytest.raises(CriteriaReadException):
            check_load("sample_2d_ok.tsv", CriteriaReadMode.FLOAT_1D, data_path)
        with pytest.raises(CriteriaReadException):
            check_load("sample_2d_bad.tsv", CriteriaReadMode.FLOAT_1D, data_path)
        with pytest.raises(CriteriaReadException):
            check_load("sample_list.tsv", CriteriaReadMode.FLOAT_1D, data_path)

        data = criteria_inputs.load_simple_tsv_lines(sample_path(data_path, "empty_tsv.tsv"), CriteriaReadMode.FLOAT_1D)
        assert data is None

    def test_load_2d_ok(self, data_path):
        check_load("sample_1d_ok.tsv", CriteriaReadMode.FLOAT_2D, data_path, length=10)
        check_load("sample_2d_ok.tsv", CriteriaReadMode.FLOAT_2D, data_path, length=5)

    def test_load_2d_bad(self, data_path):
        with pytest.raises(CriteriaReadException):
            check_load("sample_1d_bad.tsv", CriteriaReadMode.FLOAT_2D, data_path)
        with pytest.raises(CriteriaReadException):
            check_load("sample_2d_bad.tsv", CriteriaReadMode.FLOAT_2D, data_path)
        with pytest.raises(CriteriaReadException):
            check_load("sample_list.tsv", CriteriaReadMode.FLOAT_2D, data_path)

        data = criteria_inputs.load_simple_tsv_lines(sample_path(data_path, "empty_tsv.tsv"), CriteriaReadMode.FLOAT_2D)
        assert data is None

    def test_load_list(self, data_path):
        check_load("sample_1d_ok.tsv", CriteriaReadMode.JSON_LISTS, data_path, length=10)
        check_load("sample_1d_bad.tsv", CriteriaReadMode.JSON_LISTS, data_path, length=9)
        check_load("sample_2d_ok.tsv", CriteriaReadMode.JSON_LISTS, data_path, length=5)
        check_load("sample_2d_bad.tsv", CriteriaReadMode.JSON_LISTS, data_path, length=5)
        check_load("sample_list.tsv", CriteriaReadMode.JSON_LISTS, data_path, length=5)
