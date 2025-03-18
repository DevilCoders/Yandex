import csv
import logging
import os

import yaqutils.file_helpers as ufile


class SourceTsvFile(object):
    def __init__(self, file_name):
        """
        :type file_name: str
        """
        self.file_name = file_name
        self.fd = None

    def _close(self):
        assert self.fd is not None
        self.fd.close()
        self.fd = None

    def __enter__(self):
        if self.fd is not None:
            raise Exception("Cannot open source file twice")
        logging.debug("Opening source file %s", self.file_name)
        self.fd = ufile.fopen_read(self.file_name, use_unicode=False)
        return csv.reader(self.fd, delimiter="\t", lineterminator="\n", quoting=csv.QUOTE_NONE)

    def __exit__(self, *_):
        logging.debug("Closing source file %s", self.file_name)
        self._close()


class DestTsvFile(object):
    def __init__(self, file_name):
        """
        :type file_name: str
        """
        self.fd = None
        self.file_name = file_name
        self.temp_file_name = file_name + ".tmp"
        logging.debug("Opening dest file %s", self.temp_file_name)
        self.fd = ufile.fopen_write(self.temp_file_name, use_unicode=False)

    def _close(self):
        assert self.fd is not None
        self.fd.close()
        self.fd = None

    def __del__(self):
        # TODO: do not use __del__
        if self.fd is not None:
            logging.debug("Closing dest file %s", self.file_name)
            self._close()
            logging.debug("Renaming %s to %s", self.temp_file_name, self.file_name)
            os.rename(self.temp_file_name, self.file_name)
