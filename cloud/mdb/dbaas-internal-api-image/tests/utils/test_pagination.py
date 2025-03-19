"""
Pagination tests
"""
from collections import namedtuple
from datetime import datetime, timedelta

from hamcrest import assert_that, has_entry, has_key, instance_of, not_

from dbaas_internal_api.utils import pagination
from dbaas_internal_api.utils.pagination import AttributeColumn, Column
from dbaas_internal_api.utils.pagination import __name__ as PACKAGE
from dbaas_internal_api.utils.pagination import supports_pagination

#  pylint: disable=invalid-name


class Test_supports_pagination:
    """
    Test for support_pagination decorator
    """

    _items_key = 'items'

    # Here we have a lot of Python's magic
    #  pylint: disable=no-value-for-parameter
    #  pylint: disable=unused-argument
    #  pylint: disable=unexpected-keyword-arg

    def test_return_first_page(self):
        """
        Return all items if they fit one page.
        """
        result_list = [{'id': 'id-1'}, {'id': 'id-2'}]

        @supports_pagination(items_field=self._items_key, columns=[Column('id', str)])
        def _target(limit, page_token_id=None):
            assert limit is not None, 'supports_pagination should sent default limit'
            return result_list

        assert_that(
            _target(),
            has_entry(
                self._items_key,
                result_list,
            ),
        )

    def test_no_next_page_token(self):
        """
        Don't return next_page_token if we don't have
        second page
        """

        @supports_pagination(items_field=self._items_key, columns=[Column('id', str)])
        def _target(limit, page_token_id=None):
            return [{'id': 'id-1'}, {'id': 'id-2'}]

        assert_that(
            _target(page_size=100),
            not_(has_key('next_page_token')),
        )

    def test_add_next_page_token(self):
        """
        Should returns items with next_page_token
        If next page exists
        """

        @supports_pagination(items_field=self._items_key, columns=[Column('id', str)])
        def _target(limit, page_token_id=None):
            return [dict(id=i) for i in range(limit + 1)]

        assert_that(_target(), has_key('next_page_token'))

    def test_next_page_token_should_be_str(self):
        """
        next_page_token should be str
        """

        @supports_pagination(items_field=self._items_key, columns=[Column('id', str)])
        def _target(limit, page_token_id=None):
            return [dict(id=i) for i in range(limit + 1)]

        ret = _target()
        assert_that(ret['next_page_token'], instance_of(str))

    def test_return_second_page(self):
        """
        Should return token to next_page
        and this page should be accessable by that token
        """
        O = namedtuple('O', ['id'])  # noqa: E741
        data = [
            O(1),
            O(2),
            O(3),
            O(4),
        ]

        @supports_pagination(items_field=self._items_key, columns=[AttributeColumn('id', int)])
        def _target(limit: int, page_token_id=None):
            return [d for d in data if page_token_id is None or d.id > page_token_id][:limit]

        first_page = _target(page_size=3)
        assert_that(
            first_page,
            has_entry(
                self._items_key,
                [O(1), O(2), O(3)],
            ),
        )
        second_page = _target(page_token=first_page['next_page_token'])
        assert_that(
            second_page,
            has_entry(
                self._items_key,
                [O(4)],
            ),
        )

    def test_return_third_page(self):
        """
        Got so pythonics troubles when calculate limit

        Test for case, when we get limit
        from page_token
        """
        O = namedtuple('O', ['id'])  # noqa: E741
        data = [
            O(1),
            O(2),
            O(3),
            O(4),
            O(5),
            O(6),
            O(7),
        ]

        @supports_pagination(items_field=self._items_key, columns=[AttributeColumn('id', int)])
        def _target(limit: int, page_token_id=None):
            return [d for d in data if page_token_id is None or d.id > page_token_id][:limit]

        first_page = _target(page_size=2)
        second_page = _target(page_size=2, page_token=first_page['next_page_token'])
        assert_that(second_page, has_key('next_page_token'))
        third_page = _target(page_size=2, page_token=second_page['next_page_token'])
        assert_that(third_page, has_key('next_page_token'))

    def test_second_page_with_composite_columns(self):
        """
        Test get second page for 2 handler with 2 columns
        """
        O = namedtuple('O', ['str', 'payload', 'date'])  # noqa: E741
        data = [O(str='test', payload='data %r' % i, date=datetime.now() + timedelta(days=i)) for i in range(4)]

        @supports_pagination(
            items_field=self._items_key,
            columns=[
                AttributeColumn('date', pagination.date),
                AttributeColumn('str', str),
            ],
        )
        def _target(limit, page_token_date=None, page_token_str=None):
            data_filtered = data
            if page_token_date is not None:
                assert isinstance(page_token_date, datetime)
                assert isinstance(page_token_str, str)
                data_filtered = [d for d in data if (d.date, d.str) > (page_token_date, page_token_str)]
            return data_filtered[:limit]

        first_page = _target(page_size=2)
        assert_that(
            first_page,
            has_entry(
                self._items_key,
                [data[0], data[1]],
            ),
        )
        second_page = _target(page_size=2, page_token=first_page['next_page_token'])
        assert_that(
            second_page,
            has_entry(
                self._items_key,
                [data[2], data[3]],
            ),
        )

    def test_dont_return_next_page_token_on_last_page(self):
        """
        Should return token to next_page
        and this page should be accessable by that token
        """
        O = namedtuple('O', ['id'])  # noqa: E741
        data = [
            O(1),
            O(2),
            O(3),
            O(4),
        ]

        @supports_pagination(items_field=self._items_key, columns=[AttributeColumn('id', int)])
        def _target(limit, page_token_id=None):
            return [d for d in data if page_token_id is None or d.id > page_token_id][:limit]

        first_page = _target(page_size=2)
        last_page_token = first_page['next_page_token']
        assert_that(
            _target(
                page_size=2,
                page_token=last_page_token,
            ),
            not_(has_key('next_page_token')),
        )

    def test_call_abort_for_bad_tokens(self, mocker):
        """
        Throw error for invalid token
        """
        abort_mock = mocker.patch(PACKAGE + '.abort')

        @supports_pagination(items_field=self._items_key, columns=[Column('id', int)])
        def _target(limit, page_token_date=None, page_token_str=None):
            return []

        _target(page_token='42')

        abort_mock.assert_called_once_with(400, message='Invalid pagination token')

    def test_required_field_missing(self, mocker):
        """
        Eat KeyError and IndexError when form page_token
        """
        abort_mock = mocker.patch(PACKAGE + '.abort')

        data_without_required_id = [{'id': 1}, {'id': 2}, {'id': 3}]

        @supports_pagination(items_field=self._items_key, columns=[Column('required_id', int)])
        def _target(limit, **_):
            return data_without_required_id[:limit]

        _target(page_size=2)

        abort_mock.assert_called_once_with(400, message='Failed to form pagination token')

    def test_eat_errors_for_attribute_columns(self, mocker):
        """
        Eat errors when form page_token from AttributeColumns too
        """

        abort_mock = mocker.patch(PACKAGE + '.abort')

        data_without_attribute = [{'id': 1}, {'id': 2}, {'id': 3}]

        @supports_pagination(items_field=self._items_key, columns=[AttributeColumn('non_existed_attribute', int)])
        def _target(limit, page_token_date=None, page_token_str=None):
            return data_without_attribute[:limit]

        _target(page_size=2)

        abort_mock.assert_called_once_with(400, message='Failed to form pagination token')
