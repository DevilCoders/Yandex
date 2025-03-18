import logging
import yaqutils.json_helpers as ujson
import yaqutils.file_helpers as ufile


class JsonLinesReader(object):
    def __init__(self, in_fd, row_class):
        """
        :type in_fd:
        :type row_class:
        """
        self.in_fd = in_fd
        self.row_class = row_class
        self.line_number = 0

    def __iter__(self):
        row = self.read_row()
        if row is None:
            raise StopIteration
        yield row

    def read_row(self):
        self.line_number += 1
        line = self.in_fd.readline()
        if not line:
            return None
        try:
            row_data = ujson.load_from_str(line)
        except Exception as exc:
            line_len = len(line)
            line_prefix = line[:500]
            line_suffix = line[-500:]
            logging.error("Cannot load json line at line number %s (length: %s). Error: %s, Prefix: '%s', suffix: '%s'",
                          self.line_number, line_len, exc, line_prefix, line_suffix)

            row_data = ujson.load_from_str(line, force_native_json=True)
            # self.dump_problem_line(line)

        return self.row_class.deserialize(row_data)

    def dump_problem_line(self, line):
        debug_file_name = "serpset-line-debug.txt"
        ufile.write_text_file(file_name=debug_file_name, file_data=line)
        logging.info("saved problem line to file %s")


class JsonLinesWriter(object):
    def __init__(self, out_fd, cache_size, sort_keys=True):
        self.out_fd = out_fd
        self.cache_size = cache_size
        self.sort_keys = sort_keys
        self.row_cache = []

    def write_row(self, ser_object):
        """
        :type ser_object:
        :rtype: None
        """
        self.row_cache.append(ser_object)
        if len(self.row_cache) > self.cache_size:
            self.flush()

    def flush(self):
        for line in self.row_cache:
            ujson.dump_to_fd(obj=line.serialize(), fd=self.out_fd, sort_keys=self.sort_keys)
            self.out_fd.write("\n")
        self.row_cache = []
