import yaqutils.file_helpers as ufile

from serp import JsonLinesWriter
from serp import JsonLinesReader

from serp import SerpQueryInfo
from serp import SerpMarkupInfo
from serp import SerpUrlsInfo

from serp import ParsedSerp


class SerpSetFileHandler(object):
    def __init__(self, queries_file, markup_file, urls_file, mode="read"):
        self.queries_file = queries_file
        self.markup_file = markup_file
        self.urls_file = urls_file

        self.queries_fd = None
        self.markup_fd = None
        self.urls_fd = None

        if mode == "read":
            self.fopen_func = ufile.fopen_read
        elif mode == "write":
            self.fopen_func = ufile.fopen_write
        else:
            raise Exception("Incorrect file open mode ('read' or 'write' expected)")

    def __enter__(self):
        if self.queries_file:
            self.queries_fd = self.fopen_func(self.queries_file)
        else:
            self.queries_fd = None

        if self.markup_file:
            self.markup_fd = self.fopen_func(self.markup_file)
        else:
            self.markup_fd = None

        if self.urls_file:
            self.urls_fd = self.fopen_func(self.urls_file)
        else:
            self.urls_fd = None

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.queries_fd is not None:
            self.queries_fd.close()

        if self.markup_fd is not None:
            self.markup_fd.close()

        if self.urls_fd is not None:
            self.urls_fd.close()


class SerpSetReader():
    def __init__(self, queries_reader, markup_reader, urls_reader):
        """
        :type queries_reader: JsonLinesReader
        :type markup_reader: JsonLinesReader | None
        :type urls_reader: JsonLinesReader | None
        """
        self.queries_reader = queries_reader
        self.markup_reader = markup_reader
        self.urls_reader = urls_reader

    @staticmethod
    def from_file_handler(file_handler):
        """
        :type file_handler: SerpSetFileHandler
        :rtype: SerpSetReader
        """
        return SerpSetReader._from_descriptors(queries_fd=file_handler.queries_fd,
                                               markup_fd=file_handler.markup_fd,
                                               urls_fd=file_handler.urls_fd)

    @staticmethod
    def _from_descriptors(queries_fd, urls_fd, markup_fd):
        """
        :type queries_fd:
        :type urls_fd: file | None
        :type markup_fd: file | None
        :rtype: SerpSetReader
        """
        queries_reader = JsonLinesReader(queries_fd, SerpQueryInfo)
        if markup_fd:
            markup_reader = JsonLinesReader(markup_fd, SerpMarkupInfo)
        else:
            markup_reader = None
        if urls_fd:
            urls_reader = JsonLinesReader(urls_fd, SerpUrlsInfo)
        else:
            urls_reader = None
        serpset_reader = SerpSetReader(queries_reader=queries_reader,
                                       urls_reader=urls_reader,
                                       markup_reader=markup_reader)
        return serpset_reader

    def __iter__(self):
        return self

    # python3
    def __next__(self):
        parsed_serp = self.read_serp()
        if parsed_serp.is_empty():
            raise StopIteration
        return parsed_serp

    # python2
    def next(self):
        return self.__next__()

    def read_serp(self):
        """
        :rtype: ParsedSerp
        """
        query_info = self.queries_reader.read_row()
        states = [query_info is not None]

        if self.markup_reader:
            markup_info = self.markup_reader.read_row()
            states.append(markup_info is not None)
        else:
            markup_info = None

        if self.urls_reader:
            urls_info = self.urls_reader.read_row()
            states.append(urls_info is not None)
        else:
            urls_info = None

        if len(set(states)) != 1:
            raise Exception("Data error: misaligned serpset files: {}".format(states))
        return ParsedSerp(query_info=query_info, markup_info=markup_info, urls_info=urls_info)


class SerpSetWriter(object):
    def __init__(self, queries_writer, markup_writer, urls_writer):
        """
        :type queries_writer: JsonLinesWriter
        :type markup_writer: JsonLinesWriter
        :type urls_writer: JsonLinesWriter
        """
        self.queries_writer = queries_writer
        self.markup_writer = markup_writer
        self.urls_writer = urls_writer

    @staticmethod
    def from_file_handler(file_handler):
        """
        :type file_handler: SerpSetFileHandler
        :rtype: SerpSetWriter
        """
        return SerpSetWriter._from_descriptors(queries_fd=file_handler.queries_fd,
                                               markup_fd=file_handler.markup_fd,
                                               urls_fd=file_handler.urls_fd)

    @staticmethod
    def _from_descriptors(queries_fd, markup_fd, urls_fd):
        """
        :type queries_fd:
        :type markup_fd:
        :type urls_fd:
        :rtype: SerpSetWriter
        """
        queries_writer = JsonLinesWriter(queries_fd, cache_size=500)
        markup_writer = JsonLinesWriter(markup_fd, cache_size=100)
        urls_writer = JsonLinesWriter(urls_fd, cache_size=100)
        serpset_writer = SerpSetWriter(queries_writer=queries_writer,
                                       markup_writer=markup_writer,
                                       urls_writer=urls_writer)

        return serpset_writer

    def write_serp(self, parsed_serp):
        """
        :type parsed_serp: ParsedSerp
        :rtype: None
        """
        self.queries_writer.write_row(parsed_serp.query_info)
        self.markup_writer.write_row(parsed_serp.markup_info)
        self.urls_writer.write_row(parsed_serp.urls_info)

    def flush(self):
        self.queries_writer.flush()
        self.markup_writer.flush()
        self.urls_writer.flush()
