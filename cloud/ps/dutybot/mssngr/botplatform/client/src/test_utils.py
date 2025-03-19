import typing as t  # noqa


class MockResponse:
    def __init__(self, data, status_code):
        # type: (t.Any, int) -> None
        self._data = data
        self.status_code = status_code

    def json(self):
        # type: () -> t.Any
        return self._data
