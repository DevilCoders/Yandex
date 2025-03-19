"""Test paging utilities"""


from yc_common.paging import read_page, page_handler


class _Item:
    def __init__(self, id, name=None):
        self.id = id
        self.name = name


def test_list_with_full_pages():
    """No need for additional queries"""

    def _lister(cursor, limit):

        class _Response:
            def __init__(self):
                self.items = [_Item(i) for i in range(3)]
                self.next_page_token = "original"

        return _Response()

    response = read_page(_lister, None, 3, "items")
    assert [i.id for i in response.items] == [0, 1, 2]
    assert response.next_page_token == "original"  # Cursor provided by original lister


def test_list_with_partial_pages_has_more():
    """Need to make additional queries to rich desired limit, always has more data"""

    def _lister(cursor, limit):

        class _Response:
            def __init__(self):
                self.items = [_Item(i) for i in range(3)]
                self.next_page_token = "original"

        return _Response()

    response = read_page(_lister, None, 5, "items")
    assert [i.id for i in response.items] == [0, 1, 2, 0, 1]
    assert response.next_page_token == 1  # Cursor provided by helper


def test_list_with_partial_pages_no_more():
    """Need to make additional queries to rich desired limit, but no more data"""

    def _lister(cursor, limit):

        class _Response:
            def __init__(self):
                self.items = [_Item(i) for i in range(3)]
                self.next_page_token = None

        return _Response()

    response = read_page(_lister, None, 5, "items")
    assert [i.id for i in response.items] == [0, 1, 2]
    assert response.next_page_token is None  # Cursor provided by original lister


def test_list_without_limit():
    """Need to make additional queries to rich desired limit"""

    def _lister(cursor, limit):

        class _Response:
            def __init__(self):
                self.items = [_Item(i) for i in range(3)]
                self.next_page_token = "original"

        return _Response()

    response = read_page(_lister, None, None, "items")
    assert [i.id for i in response.items] == [0, 1, 2]
    assert response.next_page_token == "original"


def test_page_handler():
    """Test work of page handler both with non zero cursor and additional queries"""

    @page_handler
    def _handler(cursor, limit, name):
        class _Response:
            def __init__(self):
                self.items = [_Item(i, name) for i in range(cursor, cursor + limit)]
                self.next_page_token = self.items[-1].id

        return _Response()

    response = _handler({"page_token": 2, "page_size": 5}, "name")
    assert [i.id for i in response.items] == [2, 3, 4, 5, 6]
    assert [i.name for i in response.items] == ["name"] * 5
    assert response.next_page_token == 6


def test_page_handler_custom_items():
    """Test work of page handler both with non zero cursor and additional queries"""

    @page_handler(items="myitems")
    def _handler(cursor, limit, name):
        class _Response:
            def __init__(self):
                self.myitems = [_Item(i, name) for i in range(cursor, cursor + limit)]
                self.next_page_token = self.myitems[-1].id

        return _Response()

    response = _handler({"page_token": 2, "page_size": 5}, "name")
    assert [i.id for i in response.myitems] == [2, 3, 4, 5, 6]
    assert [i.name for i in response.myitems] == ["name"] * 5
    assert response.next_page_token == 6
