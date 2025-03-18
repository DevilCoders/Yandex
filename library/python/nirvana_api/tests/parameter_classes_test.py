import pytest
from datetime import datetime
from nirvana_api.parameter_classes import PaginationData, DateRange, WorkflowFilters, \
    StatusEnum, ResultEnum, Parameter


def test_pagination_data():
    pagination_data = PaginationData(PaginationData.MIN_PAGE_SIZE, PaginationData.MIN_PAGE_NUMBER)
    assert pagination_data.pageSize == PaginationData.MIN_PAGE_SIZE
    assert pagination_data.pageNumber == PaginationData.MIN_PAGE_NUMBER


def test_page_size_lower_than_min_exception():
    with pytest.raises(ValueError):
        PaginationData(PaginationData.MIN_PAGE_SIZE - 1, PaginationData.MIN_PAGE_NUMBER)


def test_page_size_greater_than_max_exception():
    with pytest.raises(ValueError):
        PaginationData(PaginationData.MAX_PAGE_SIZE + 1, PaginationData.MIN_PAGE_NUMBER)


def test_page_number_lower_than_min_exception():
    with pytest.raises(ValueError):
        PaginationData(PaginationData.MIN_PAGE_SIZE, PaginationData.MIN_PAGE_NUMBER - 1)


def test_dates_serialization():
    from_dt = datetime(year=1989, month=9, day=9, hour=13, minute=35, second=0)
    to_dt = datetime(year=2017, month=9, day=9, hour=13, minute=35, second=0)

    dates_range = DateRange(from_dt, to_dt)
    assert dates_range.from_ == '1989-09-09T13:35:00+0300'
    assert dates_range.to == '2017-09-09T13:35:00+0300'


def test_workflow_filters_enums():
    w = WorkflowFilters(status=[StatusEnum.RUNNING], result=[ResultEnum.SUCCESS])
    assert w.status == [StatusEnum.RUNNING.value]
    assert w.result == [ResultEnum.SUCCESS.value]


def test_workflow_filters_strings_instead_of_enums_exception():
    with pytest.raises(AttributeError):
        WorkflowFilters(status=['running'], result=['success'])


def test_parameter_repr_data_int():
    class TestData(Parameter):
        def __init__(self):
            self.x = 1
    test_data = TestData()
    assert '{!r}'.format(test_data) == 'TestData(x=1)'


def test_parameter_repr_data_str():
    class TestData(Parameter):
        def __init__(self):
            self.x = 'test'
    test_data = TestData()
    assert '{!r}'.format(test_data) == 'TestData(x=\'test\')'
