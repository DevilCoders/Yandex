from core.scrapped_data import ScrappedData


class NotSupportedException(Exception):

    def __init__(self, *args: object) -> None:
        super().__init__(*args)


class ScrappingException(Exception):

    def __init__(self, scrapped_data: ScrappedData = None, ex: Exception = None, message: str = None):
        self._data = scrapped_data
        self._ex = ex
        self._message = message

    def __repr__(self) -> str:
        if self._data is None and self._ex is None:
            return f"ScrappingFailedException: {self._message}"

        if self._message is None:
            return f"Fail to get '{self._data.key}'. Exception: {str(self._ex)}"
        else:
            return f"Fatal error when attempting to get {self._data.key}. Exception message: {str(self._ex)}"

    def __str__(self) -> str:
        return self.__repr__()
