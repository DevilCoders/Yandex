# coding=utf-8

import logging
import pytest
import os
import shutil

import yaqutils.test_helpers as utest
import yaqutils.file_helpers as ufile
import yaqutils.six_helpers as usix
import serp.field_extender as sfe
import serp.keys_dumper as skd

from serp import FieldExtenderKey
from serp import DumpSettings
from serp import ExtendSettings
from serp import QueryKey


def get_data_file_path(data_path, filename):
    return os.path.join(data_path, "extender", filename)


# noinspection PyClassHasNoInit
class TestFieldExtenderKey:
    def test_field_extender_key_repr(self):
        query_key = QueryKey(query_text=u"тест", query_region=0,
                             query_country=None, query_device=0, query_uid="y123")
        fe_key = FieldExtenderKey(query_key=query_key, url="url", pos=None)
        logging.info("%s", fe_key)


def run_get_keys(dump_settings, keys_file_name, tmpdir, data_path):
    if dump_settings.query_mode:
        queries_file = get_data_file_path(data_path, "test-queries-query-mode.jsonl")
        urls_file = None
    else:
        queries_file = get_data_file_path(data_path, "test-queries.jsonl")
        urls_file = get_data_file_path(data_path, "test-urls.jsonl")

    keys_tsv = skd.get_all_keys_single(queries_file=queries_file,
                                       urls_file=urls_file,
                                       dump_settings=dump_settings,
                                       serpset_id="100500")

    output_keys_file = str(tmpdir.join(keys_file_name))

    ufile.save_tsv(keys_tsv, output_keys_file)
    utest.diff_files(test_file=output_keys_file, expected_file=get_data_file_path(data_path, keys_file_name))


# noinspection PyClassHasNoInit
class TestGetKeys:
    def test_get_keys_url_mode(self, tmpdir, data_path):
        dump_settings = DumpSettings(with_serpset_id=False, query_mode=False, with_position=True,
                                     with_qid=True, with_device=True, with_country=True, with_uid=True,
                                     serp_depth=None)

        run_get_keys(dump_settings=dump_settings, keys_file_name="test-keys.tsv", tmpdir=tmpdir, data_path=data_path)

    def test_get_keys_url_mode_limited_depth(self, tmpdir, data_path):
        dump_settings = DumpSettings(with_serpset_id=False, query_mode=False, with_position=True,
                                     with_qid=True, with_device=True, with_country=True, with_uid=True,
                                     serp_depth=2)

        run_get_keys(dump_settings=dump_settings, keys_file_name="test-keys-limited-depth.tsv", tmpdir=tmpdir,
                     data_path=data_path)

    def test_get_keys_query_mode(self, tmpdir, data_path):
        dump_settings = DumpSettings(with_serpset_id=False, query_mode=True, with_position=False,
                                     with_qid=True, with_device=True, with_country=True, with_uid=True,
                                     serp_depth=None)

        run_get_keys(dump_settings=dump_settings, keys_file_name="test-keys-query-mode.tsv", tmpdir=tmpdir,
                     data_path=data_path)

    def test_get_keys_query_mode_partial_key_no_uid(self, tmpdir, data_path):
        # uid differs for qid=4
        dump_settings = DumpSettings(with_serpset_id=False, query_mode=True, with_position=False,
                                     with_qid=False, with_device=True, with_country=True, with_uid=False,
                                     serp_depth=None)

        run_get_keys(dump_settings=dump_settings, keys_file_name="test-keys-no-uid.tsv", tmpdir=tmpdir,
                     data_path=data_path)

    def test_get_keys_query_mode_partial_key_no_country_no_device(self, tmpdir, data_path):
        # country differs for qid=1
        dump_settings = DumpSettings(with_serpset_id=False, query_mode=True, with_position=False,
                                     with_qid=False, with_device=False, with_country=False, with_uid=True,
                                     serp_depth=None)

        run_get_keys(dump_settings=dump_settings, keys_file_name="test-keys-no-country-no-device.tsv", tmpdir=tmpdir,
                     data_path=data_path)


class DefaultModeExtender(object):
    @staticmethod
    def extend(extend_params):
        """
        :type extend_params: ExtendParamsForAPI
        :rtype: None
        """
        extend_params.scales[extend_params.field_name] = extend_params.custom_value


def run_extend(extend_settings, field_values, expected_markup_filename, tmpdir, data_path, extender=None):
    assert tmpdir is not None

    if extend_settings.query_mode:
        queries_file = get_data_file_path(data_path, "test-queries-query-mode.jsonl")
        markup_file = get_data_file_path(data_path, "test-markup-query-mode.jsonl")
        urls_file = None
    else:
        queries_file = get_data_file_path(data_path, "test-queries.jsonl")
        markup_file = get_data_file_path(data_path, "test-markup.jsonl")
        urls_file = get_data_file_path(data_path, "test-urls.jsonl")

    tmp_markup_file = str(tmpdir.join("test-markup-updated.jsonl"))
    shutil.copyfile(markup_file, tmp_markup_file)

    sfe.extend_one_serpset(serpset_id="100500",
                           queries_file=queries_file,
                           markup_file=tmp_markup_file,
                           urls_file=urls_file,
                           field_values=field_values,
                           extend_settings=extend_settings,
                           extender=extender)

    utest.diff_files(test_file=tmp_markup_file, expected_file=get_data_file_path(data_path, expected_markup_filename))


# noinspection PyClassHasNoInit
class TestAddField:
    def test_extend_serpset_url_mode_default(self, tmpdir, data_path):
        extend_settings = ExtendSettings(query_mode=False,
                                         with_position=False,
                                         field_name="custom",
                                         overwrite=False,
                                         flat_mode=False)

        key1 = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер раз", query_region=1),
                                url="http://example2.com", pos=None)

        key2 = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер два", query_region=1),
                                url="http://example4.com", pos=None)
        field_values = {
            key1: u"какой-то текст",
            key2: 100500,
        }

        run_extend(extend_settings=extend_settings, field_values=field_values,
                   expected_markup_filename="updated-markup.jsonl", tmpdir=tmpdir, data_path=data_path)

    def test_extend_serpset_url_mode_flat(self, tmpdir, data_path):
        extend_settings = ExtendSettings(query_mode=False,
                                         with_position=False,
                                         field_name="prefix_",
                                         overwrite=False,
                                         flat_mode=True)

        key1 = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер раз", query_region=1),
                                url="http://example2.com", pos=None)

        key2 = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер два", query_region=1),
                                url="http://example4.com", pos=None)
        field_values = {
            key1: {"user_scale1": u"какой-то текст", "user_scale2": 123},
            key2: {"user_scale1": 456, "user_scale2": 100500},
        }

        run_extend(extend_settings=extend_settings, field_values=field_values,
                   expected_markup_filename="updated-markup-flat.jsonl", tmpdir=tmpdir, data_path=data_path)

    def test_extend_serpset_url_mode_custom(self, tmpdir, data_path):
        extend_settings = ExtendSettings(query_mode=False,
                                         with_position=False,
                                         field_name="custom",
                                         overwrite=False,
                                         flat_mode=False)

        key1 = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер раз", query_region=1),
                                url="http://example2.com", pos=None)

        key2 = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер два", query_region=1),
                                url="http://example4.com", pos=None)
        field_values = {
            key1: u"какой-то текст",
            key2: 100500,
        }

        extender = DefaultModeExtender()
        # result for this custom extender should be same as in 'test_extend_serpset_url_mode_default'
        run_extend(extend_settings=extend_settings, field_values=field_values,
                   expected_markup_filename="updated-markup.jsonl", tmpdir=tmpdir,
                   extender=extender, data_path=data_path)

    def test_extend_serpset_no_overwrite_query_mode(self, tmpdir, data_path):
        extend_settings = ExtendSettings(query_mode=True,
                                         with_position=False,
                                         overwrite=False,
                                         field_name="serp-field",
                                         flat_mode=False)

        key = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер раз", query_region=1),
                               url="-", pos=None)

        field_values = {
            key: 'this text should not replace serp_data["serp-field"]'
        }

        with pytest.raises(Exception):
            run_extend(extend_settings=extend_settings, field_values=field_values,
                       expected_markup_filename=None, tmpdir=tmpdir, data_path=data_path)

    def test_extend_serpset_no_overwrite_url_mode(self, tmpdir, data_path):
        extend_settings = ExtendSettings(query_mode=False,
                                         with_position=False,
                                         overwrite=False,
                                         field_name="RELEVANCE",
                                         flat_mode=False)

        key = FieldExtenderKey(query_key=QueryKey(query_text=u"текст запроса номер раз", query_region=1),
                               url="http://example2.com", pos=None)

        field_values = {
            key: "this text should NOT replace relevance value"
        }

        with pytest.raises(Exception):
            run_extend(extend_settings=extend_settings, field_values=field_values,
                       expected_markup_filename=None, tmpdir=tmpdir, data_path=data_path)

    def test_extend_serpset_query_mode_func(self, tmpdir, data_path):
        extend_settings = ExtendSettings(query_mode=True,
                                         with_position=False,
                                         overwrite=False,
                                         field_name="custom",
                                         flat_mode=False)

        query_key1 = QueryKey(query_text=u"текст запроса номер раз", query_region=1)
        key1 = FieldExtenderKey(query_key=query_key1, url="-", pos=None)

        query_key2 = QueryKey(query_text=u"текст запроса номер два", query_region=1)
        key2 = FieldExtenderKey(query_key=query_key2, url="-", pos=None)

        query_key3 = QueryKey(query_text=u"текст запроса номер три", query_region=2)
        key3 = FieldExtenderKey(query_key=query_key3, url="-", pos=None)

        query_key4 = QueryKey(query_text="non-existing-query", query_region=1)
        key4 = FieldExtenderKey(query_key=query_key4, url="-", pos=None)

        field_values = {
            key1: u"какой-то текст",
            key2: 100500,
            key3: "region-not-match-and-this-should-NOT-be-added",
            key4: "query-text-not-match-and-this-should-NOT-be-added",
        }

        run_extend(extend_settings=extend_settings, field_values=field_values,
                   expected_markup_filename="updated-markup-query-mode.jsonl", tmpdir=tmpdir, data_path=data_path)

    def test_extend_serpset_query_mode_from_file(self, tmpdir, data_path):
        extend_settings = ExtendSettings(query_mode=True,
                                         with_position=False,
                                         overwrite=False,
                                         field_name="custom",
                                         flat_mode=False)

        # If it's loaded from file, query_text originally has 'str' type, not 'unicode'.
        # This yields MSTAND-960.
        # previous test creates QueryKey manually and does not test this case.
        enrichment_file = get_data_file_path(data_path, "enrichment-query-mode.tsv")
        field_values = sfe.load_enrichment_tsv_file(enrichment_file, extend_settings)

        run_extend(extend_settings=extend_settings, field_values=field_values,
                   expected_markup_filename="updated-markup-query-mode.jsonl", tmpdir=tmpdir, data_path=data_path)


# noinspection PyClassHasNoInit
class TestCustomDataParser:
    def test_parse_tsv_good(self, data_path):
        extend_settings = ExtendSettings(query_mode=False,
                                         with_position=False,
                                         overwrite=False,
                                         flat_mode=False,
                                         field_name="should-not-be-used")

        custom_file = get_data_file_path(data_path, "custom-data-good.tsv")
        with ufile.fopen_read(custom_file) as tsv_fd:
            custom_data = {}
            sfe.parse_enrichment_tsv_file(custom_data, tsv_fd, extend_settings)

            for key, value in usix.iteritems(custom_data):
                logging.debug("custom_data[%s] = '%s'", key, value)

            query_key1 = QueryKey(query_text="query-text-1", query_region=1)
            key1 = FieldExtenderKey(query_key=query_key1, url="http://example1.com", pos=None)

            assert custom_data[key1] == "custom-value-1"

            query_key2 = QueryKey(query_text="query-text-2", query_region=1)
            key2 = FieldExtenderKey(query_key=query_key2, url="http://example2.com", pos=None)
            assert (custom_data[key2] == 100500)

    def test_parse_tsv_bad_columns(self, data_path):
        extend_settings = ExtendSettings(query_mode=False,
                                         with_position=False,
                                         overwrite=False,
                                         field_name="should-not-be-used",
                                         flat_mode=False)

        custom_file = get_data_file_path(data_path, "custom-data-bad.tsv")
        with ufile.fopen_read(custom_file) as tsv_fd:
            with pytest.raises(sfe.ParseError):
                custom_data = {}
                sfe.parse_enrichment_tsv_file(custom_data, tsv_fd, extend_settings)

    def test_parse_tsv_bad_json(self, data_path):
        extend_settings = ExtendSettings(query_mode=False,
                                         with_position=False,
                                         overwrite=False,
                                         field_name="should-not-be-used",
                                         flat_mode=False)

        custom_file = get_data_file_path(data_path, "custom-data-bad-json.tsv")
        with ufile.fopen_read(custom_file) as tsv_fd:
            with pytest.raises(Exception):
                custom_data = {}
                sfe.parse_enrichment_tsv_file(custom_data, tsv_fd, extend_settings)

    def test_extend_settings_bad_empty_field_name(self):
        with pytest.raises(Exception):
            ExtendSettings(query_mode=False,
                           with_position=False,
                           field_name="",
                           overwrite=False,
                           flat_mode=False)

    def test_extend_settings_query_mode_and_position(self):
        with pytest.raises(Exception):
            ExtendSettings(query_mode=True,
                           with_position=True,
                           field_name="",
                           overwrite=False,
                           flat_mode=False)
