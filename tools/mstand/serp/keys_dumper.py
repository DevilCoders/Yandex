import logging
import time

import yaqutils.misc_helpers as umisc

from serp import SerpSetFileHandler
from serp import SerpSetReader
from serp import DumpSettings  # noqa
from serp import ParsedSerpDataStorage  # noqa


def get_all_keys(serpset_ids, parsed_serp_storage, dump_settings):
    """
    :type serpset_ids: list[str]
    :type parsed_serp_storage: ParsedSerpDataStorage
    :type dump_settings: DumpSettings
    :rtype:
    """
    start_time = time.time()
    all_keys = set()

    for index, serpset_id in enumerate(serpset_ids):
        logging.info("Extracting keys from serpset %s", serpset_id)
        if not parsed_serp_storage.is_already_parsed(serpset_id):
            raise Exception("No parsed serpset data for serpset {}. Do you forget to run parser?".format(serpset_id))
        queries_file = parsed_serp_storage.queries_by_serpset(serpset_id)
        if not dump_settings.query_mode:
            urls_file = parsed_serp_storage.urls_by_serpset(serpset_id)
        else:
            urls_file = None

        for tsv_key in get_all_keys_single(queries_file=queries_file,
                                           urls_file=urls_file,
                                           dump_settings=dump_settings,
                                           serpset_id=serpset_id):
            if dump_settings.with_serpset_id:
                tsv_key += (serpset_id,)
            all_keys.add(tsv_key)
        logging.info("Serpset %s processed", serpset_id)
        umisc.log_progress("keys extraction", index=index, total=len(serpset_ids))
    sorted_keys_list = list(sorted(all_keys))
    umisc.log_elapsed(start_time, "total records extracted: %d", len(sorted_keys_list))
    return sorted_keys_list


# extracts all keys from one serpset
def get_all_keys_single(queries_file, urls_file, dump_settings, serpset_id):
    """
    :type queries_file: str
    :type urls_file: str | None
    :type dump_settings: DumpSettings
    :type serpset_id: str
    :rtype: list[tuple]
    """
    serpset_file_handler = SerpSetFileHandler(queries_file=queries_file, urls_file=urls_file,
                                              markup_file=None, mode="read")
    with serpset_file_handler:
        serpset_reader = SerpSetReader.from_file_handler(serpset_file_handler)
        return get_all_keys_from_serpset(serpset_id=serpset_id, dump_settings=dump_settings,
                                         serpset_reader=serpset_reader)


def get_all_keys_from_serpset(serpset_id, dump_settings, serpset_reader):
    """
    :type serpset_id: str
    :type dump_settings: DumpSettings
    :type serpset_reader: SerpSetReader
    :rtype: list[tuple]
    """
    one_serpset_keys = set()

    for parsed_serp in serpset_reader:
        # markup_info is not used (it's always None)
        query_info = parsed_serp.query_info
        urls_info = parsed_serp.urls_info
        qid = query_info.qid

        query_part = query_info.query_key.to_tuple(dump_settings)
        if dump_settings.query_mode:
            fake_url = "-"
            key = query_part + (fake_url,)
            if dump_settings.with_qid:
                key += (qid,)
            one_serpset_keys.add(key)
        else:
            assert urls_info
            if dump_settings.serp_depth is None:
                limited_results = urls_info.res_urls
            else:
                limited_results = urls_info.res_urls[:dump_settings.serp_depth]

            for rindex, res_url in enumerate(limited_results):
                url = res_url.url
                pos = res_url.pos
                assert res_url.pos == pos
                key = query_part + (url,)
                if dump_settings.with_position:
                    key += (pos,)
                if dump_settings.with_qid:
                    key += (qid,)
                one_serpset_keys.add(key)
    logging.info("extracted %d records from serpset %s", len(one_serpset_keys), serpset_id)
    return list(sorted(one_serpset_keys))
