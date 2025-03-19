import yt.wrapper as yt


class YtReader:
    def __init__(self, table_path, columns, start_index=0, end_index=None):
        self._table_path = table_path
        self._iterator = None

        self._start_index = start_index
        self._end_index = end_index
        self._columns = columns

    def _reset_reader(self):
        table = yt.TablePath(name=self._table_path, columns=self._columns, start_index=self._start_index, end_index=self._end_index)
        self._iterator = yt.read_table(table=table, format=yt.YsonFormat(encoding=None))

    def __next__(self):
        if self._iterator is None:
            self._reset_reader()

        try:
            return next(self._iterator)
        except StopIteration:
            raise StopIteration

    def __iter__(self):
        return self
