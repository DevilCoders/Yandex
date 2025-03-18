import yaqutils.file_helpers as ufile

from serp import JsonLinesReader
from serp import JsonLinesWriter


class Row(object):
    def __init__(self, value):
        self.value = value

    def serialize(self):
        return {"key": self.value}

    @staticmethod
    def deserialize(data):
        return Row(data["key"])


# noinspection PyClassHasNoInit
class TestJsonLinesReaderWriter:
    def test_json_lines_reader_and_writer(self, tmpdir):
        file_name = str(tmpdir.join("test.jsonl"))
        with ufile.fopen_write(file_name) as out_fd:
            writer = JsonLinesWriter(out_fd=out_fd, cache_size=3)
            for x in range(10):
                writer.write_row(Row(x))
            writer.flush()

        with ufile.fopen_read(file_name) as in_fd:
            reader = JsonLinesReader(in_fd=in_fd, row_class=Row)
            for x, row in enumerate(reader):
                assert x == row.value
