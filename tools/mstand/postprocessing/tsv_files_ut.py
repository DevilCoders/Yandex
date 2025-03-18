import os
import random

import pytest

import yaqutils.tsv_helpers as utsv
from postprocessing import DestTsvFile
from postprocessing import SourceTsvFile


# noinspection PyClassHasNoInit
class TestTsvFiles:
    def test_source_tsv_file(self, data_path):
        src_file = os.path.join(data_path, "postproc_engine", "tsv_sample.tsv")
        tsv = SourceTsvFile(src_file)
        with tsv as tsv_reader:
            lines = list(tsv_reader)
            assert len(lines) == 4
            for index, line in enumerate(lines):
                assert len(line) == index + 1

        # check if 2-nd pass over file works too
        with tsv as tsv_reader:
            lines = list(tsv_reader)
            assert len(lines) == 4

        with pytest.raises(Exception):
            with tsv as tsv_reader1:
                with tsv as tsv_reader2:
                    assert tsv_reader1
                    assert tsv_reader2

    def test_source_tsv_not_existing(self):
        with pytest.raises(Exception):
            SourceTsvFile("path/to/nonexisting/file{}".format(random.random(1000000)))

    def test_dest_tsv_file(self, tmpdir):
        dest_file = str(tmpdir.join("tsv_sample.tsv"))

        tsv_writer = DestTsvFile(dest_file)
        utsv.dump_row_to_fd([1], tsv_writer.fd)
        utsv.dump_row_to_fd([2, 3], tsv_writer.fd)
        utsv.dump_row_to_fd([4, 5, 6], tsv_writer.fd)
        utsv.dump_row_to_fd([7, 8, 9], tsv_writer.fd)
        # check atomic rename
        assert not os.path.exists(dest_file)
        del tsv_writer
        assert os.path.exists(dest_file)

        validate_tsv = SourceTsvFile(dest_file)
        with validate_tsv as tsv_reader:
            lines = list(tsv_reader)
            assert len(lines) == 4
