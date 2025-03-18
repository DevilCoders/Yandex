import os

from serp import SerpAttrs
from serp import SerpsetParseContext
from serp import PoolParseContext

from serp import SerpSetFileHandler
from serp import SerpSetWriter

from serp import RawSerpDataStorage
from serp import ParsedSerpDataStorage

import serp.serp_helpers as shelp
import serp.serpset_converter as ss_conv
import serp.serpset_parser as ssp

import yaqutils.file_helpers as ufile
import yaqutils.test_helpers as utest
from serp import DummyLock


def check_file(expected_file_name, test_file_path):
    expected_file_path = os.path.join(utest.get_source_path("serp/data/parser"), expected_file_name)
    utest.diff_files(expected_file_path, test_file_path)


# noinspection PyClassHasNoInit
class TestParseSerpFile:
    allow_no_position = True

    def test_serpset_parser(self, tmpdir, data_path):
        # store serpset for parsing in 'original' format to make editing easier.
        raw_serpset_file = os.path.join(data_path, "parser/test-serpset.json")
        converted_serpset_file = str(tmpdir.join("test-serpset.jsonl"))

        ss_conv.convert_serpset_to_jsonlines(serpset_id="100500", raw_file=raw_serpset_file, jsonlines_file=converted_serpset_file,
                                             do_unpack=False, sort_keys=True, remove_original=False,
                                             use_external_convertor=False)

        queries_file = str(tmpdir.join("test-queries.jsonl"))
        markup_file = str(tmpdir.join("test-markup.jsonl"))
        urls_file = str(tmpdir.join("test-urls.jsonl"))

        serp_reqs = {"metric.onlySearchResult.pfound"}
        judgements = {"RELEVANCE", "TW_HOST"}
        serp_attrs = SerpAttrs(serp_reqs=serp_reqs, judgements=judgements)
        # no storages here
        pool_ctx = PoolParseContext(serp_attrs=serp_attrs, allow_no_position=self.allow_no_position)
        serpset_ctx = SerpsetParseContext(pool_ctx, "100500")

        serpset_file_handler = SerpSetFileHandler(queries_file=queries_file, markup_file=markup_file,
                                                  urls_file=urls_file, mode="write")

        with serpset_file_handler:
            serpset_writer = SerpSetWriter.from_file_handler(serpset_file_handler)
            with ufile.fopen_read(converted_serpset_file) as serpset_fd:
                qid_map = {}
                lock = DummyLock()
                ssp.parse_serpset_jsonlines(serpset_ctx, serpset_fd, qid_map, lock, serpset_writer)

            serpset_writer.flush()

        expected_queries_file = os.path.join(data_path, "parser/test-queries.jsonl")
        expected_urls_file = os.path.join(data_path, "parser/test-urls.jsonl")
        expected_markup_file = os.path.join(data_path, "parser/test-markup.jsonl")
        utest.diff_files(test_file=queries_file, expected_file=expected_queries_file)
        utest.diff_files(test_file=urls_file, expected_file=expected_urls_file)
        utest.diff_files(test_file=markup_file, expected_file=expected_markup_file)

    def test_serpset_parser_on_pool(self, tmpdir, data_path):
        # just check that parser works, in common.
        tmp_root_dir = str(tmpdir)

        raw_serpset_file = os.path.join(data_path, "parser/test-serpset.json")
        converted_serpset_file = str(tmpdir.join("serpset_100500.jsonl"))

        ss_conv.convert_serpset_to_jsonlines(serpset_id="100500", raw_file=raw_serpset_file, jsonlines_file=converted_serpset_file,
                                             do_unpack=False, sort_keys=True, remove_original=False,
                                             use_external_convertor=False)

        raw_serp_storage = RawSerpDataStorage(root_dir=tmp_root_dir, cache_subdir="",
                                              use_cache=True, raw_serpset_dir_name="")
        parsed_serp_storage = ParsedSerpDataStorage(root_dir=tmp_root_dir, cache_subdir="",
                                                    use_cache=True, parsed_serpsets_dir_name="")

        pool = shelp.build_mc_pool(["100500"])
        judgements = {"RELEVANCE", "TW_HOST"}
        serp_attrs = SerpAttrs(judgements=judgements)
        pool_ctx = PoolParseContext(serp_attrs=serp_attrs, raw_serp_storage=raw_serp_storage,
                                    parsed_serp_storage=parsed_serp_storage, allow_no_position=self.allow_no_position)
        ssp.parse_serpsets(pool, pool_parse_ctx=pool_ctx)


# noinspection PyClassHasNoInit
class TestCorruptedSerpFile:
    def test_corrupted_serpset(self, data_path):
        serpset_file = os.path.join(data_path, "parser/test-corrupted-utf8.jsonl")
        judgements = {"RELEVANCE", "TW_HOST"}
        serp_attrs = SerpAttrs(judgements=judgements)
        pool_ctx = PoolParseContext(serp_attrs=serp_attrs)
        serp_ctx = SerpsetParseContext(pool_ctx, "100500")

        with ufile.fopen_read(serpset_file, use_unicode=False) as serpset_fd:
            query_id_map = {}
            lock = DummyLock()
            ssp.parse_serpset_jsonlines(serp_ctx, serpset_fd, query_id_map, lock, serpset_writer=None)
