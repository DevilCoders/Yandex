# coding=utf-8

import logging
import multiprocessing
import os
import traceback
from typing import Set

import serp.serpset_parser_single as sp_single
import yaqutils.json_helpers as ujson
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc

from serp import ParsedSerpDataStorage  # noqa
from serp import PoolParseContext  # noqa
from serp import SerpSetFileHandler
from serp import SerpSetWriter
from serp import SerpsetParseContext
from serp import DummyLock


# for adhoc jupyter computations
def parse_serpset_raw(ctx, raw_serpset_file_name):
    """
    :type ctx: SerpsetParseContext
    :type raw_serpset_file_name: str
    :rtype: list[ParsedSerp]
    """
    logging.info("loading serpset from file %s", raw_serpset_file_name)
    serpset_data = ujson.load_from_file(raw_serpset_file_name)
    logging.info("serpset %s loaded", raw_serpset_file_name)

    return parse_serpset_data(ctx=ctx, serpset_data=serpset_data)


def parse_serpset_data(ctx, serpset_data):
    """
    :type ctx: SerpsetParseContext
    :type serpset_data: list
    :rtype: list[ParsedSerp]
    """
    qid_map = {}
    lock = DummyLock()
    line_number = 0
    total_serps = 0
    failed_serps = 0

    parsed_serpset = []

    for one_serp in serpset_data:
        line_number += 1
        try:
            if line_number % 1000 == 0:
                logging.info("Processed %d lines of serpset %s", line_number, ctx.serpset_id)

            parsed_serp = sp_single.parse_one_serp(ctx=ctx, one_serp=one_serp, qid_map=qid_map, lock=lock,
                                                   serpset_line_num=line_number)

            parsed_serpset.append(parsed_serp)

            if parsed_serp.is_failed():
                failed_serps += 1
            total_serps += 1
        except Exception as exc:
            logging.error("Error parsing serpset %s at line %d: %s", ctx.serpset_id, line_number, exc)
            raise

    logging.info("Serpset %s stats, total: %s serps, failed: %s serps", ctx.serpset_id, total_serps, failed_serps)
    return parsed_serpset


def parse_serpset_jsonlines(ctx, jsonlines_serpset_fd, qid_map, lock, serpset_writer):
    """
    :type ctx: SerpsetParseContext
    :type jsonlines_serpset_fd:
    :type qid_map: dict[QueryKey, int]
    :type lock:
    :type serpset_writer: SerpSetWriter | None
    :rtype: None
    """
    line_number = 0
    total_serps = 0
    failed_serps = 0
    for query_line in jsonlines_serpset_fd:
        line_number += 1
        try:
            one_serp = ujson.load_from_str(query_line)
            if line_number % 1000 == 0:
                logging.info("Processed %d lines of serpset %s", line_number, ctx.serpset_id)

            parsed_serp = sp_single.parse_one_serp(ctx=ctx, one_serp=one_serp, qid_map=qid_map, lock=lock,
                                                   serpset_line_num=line_number)
            if serpset_writer is not None:
                serpset_writer.write_serp(parsed_serp=parsed_serp)

            if parsed_serp.is_failed():
                failed_serps += 1
            total_serps += 1
        except Exception as exc:
            logging.error("Error parsing serpset %s at line %d: %s", ctx.serpset_id, line_number, exc)
            raise

    logging.info("Serpset %s stats, total: %s serps, failed: %s serps", ctx.serpset_id, total_serps, failed_serps)


def mp_parse_one_serpset(arg):
    """
    :type arg: tuple[SerpsetParseContext, dict, threading.Lock]
    :rtype:
    """
    serpset_ctx, query_id_map, lock = arg
    try:
        parse_one_serpset_worker(serpset_ctx, query_id_map, lock)
        return arg, None
    except BaseException as exc:
        logging.info("Error occured in mp_parse_one_serpset(): %s, details: %s", exc, traceback.format_exc())
        return arg, exc


def parse_one_serpset_worker(serpset_ctx, qid_map, lock):
    """
    :type serpset_ctx: SerpsetParseContext
    :type qid_map: dict
    :type lock:
    :rtype: None
    """
    serpset_id = serpset_ctx.serpset_id
    pool_ctx = serpset_ctx.pool_parse_ctx
    pss = pool_ctx.parsed_serp_storage

    queries_file = pss.queries_by_serpset(serpset_id)
    urls_file = pss.urls_by_serpset(serpset_id)
    markup_file = pss.markup_by_serpset(serpset_id)

    dest_files = [queries_file, urls_file, markup_file]
    tmp_files = ["{}.tmp".format(fname) for fname in dest_files]
    queries_tmp, urls_tmp, markup_tmp = tmp_files

    jsonlines_serpset_file = pool_ctx.raw_serp_storage.jsonlines_serpset_by_id(serpset_id)
    logging.info('Parsing serpset %s file %s', serpset_id, jsonlines_serpset_file)

    if not os.path.exists(jsonlines_serpset_file):
        raise Exception('Serpset file {} is missing (cache is broken).'.format(jsonlines_serpset_file))

    serpset_file_handler = SerpSetFileHandler(queries_file=queries_tmp, markup_file=markup_tmp,
                                              urls_file=urls_tmp, mode="write")

    with serpset_file_handler:
        serpset_writer = SerpSetWriter.from_file_handler(serpset_file_handler)

        # MSTAND-624: load rows regardless of encoding, decode each line separately as utf-8
        # codecs.readline() breaks on Unicode NEL character
        with ufile.fopen_read(jsonlines_serpset_file, use_unicode=False) as jsonlines_serpset_fd:
            parse_serpset_jsonlines(ctx=serpset_ctx, jsonlines_serpset_fd=jsonlines_serpset_fd, qid_map=qid_map,
                                    lock=lock, serpset_writer=serpset_writer)

        serpset_writer.flush()
        logging.info("Serpset parsing done %s", serpset_id)

    logging.info("Renaming temp files for serpset %s.", serpset_id)
    for tmp_file, dest_file in zip(tmp_files, dest_files):
        os.rename(tmp_file, dest_file)

    if pool_ctx.remove_raw_serpsets:
        logging.info("raw serpset removal requested. deleting %s", jsonlines_serpset_file)
        os.unlink(jsonlines_serpset_file)

    logging.info("Serpset data for serpset %s saved to %s", serpset_id, dest_files)


def is_pool_already_parsed(serpset_ids: Set[str], parsed_serp_storage: ParsedSerpDataStorage) -> bool:
    if not parsed_serp_storage.use_cache:
        logging.info("Cache is disabled, recalculating pool completely.")
        return False

    existing_serpset_ids = set()

    for serpset_id in serpset_ids:
        if parsed_serp_storage.is_already_parsed(serpset_id):
            existing_serpset_ids.add(serpset_id)
            logging.info('Parsed serp data for serpset %s taken from cache.', serpset_id)

    logging.info("Existing parsed serpsets: %s", list(existing_serpset_ids))
    logging.info("All serpsets: %s", list(serpset_ids))
    if existing_serpset_ids < set(serpset_ids):
        return False
    return True


def is_serp_matches(serp_data, line_num, serp_num, text_pattern):
    if serp_num is not None:
        # match by line number
        return line_num == serp_num
    elif text_pattern:
        return text_pattern in serp_data
    else:
        raise Exception("Both params are not set: line number and text pattern")


def extract_one_serp(serpset_file_or_id, serp_file, serp_num=1, text_pattern=None):
    """
    :type serpset_file_or_id: str
    :type serp_file: str
    :type serp_num: int
    :type text_pattern: unicode | None
    :rtype: None
    """
    if serpset_file_or_id.isdigit():
        from serp import RawSerpDataStorage
        rds = RawSerpDataStorage()
        serpset_file = rds.jsonlines_serpset_by_id(serpset_file_or_id)
        logging.info("You've probably passed serpset-id as input file, assuming it's %s", serpset_file)
    else:
        serpset_file = serpset_file_or_id

    line_num = 1

    with ufile.fopen_read(serpset_file, use_unicode=True) as raw_serpset_fd:
        for serp_data in raw_serpset_fd:
            if is_serp_matches(serp_data, line_num, serp_num=serp_num, text_pattern=text_pattern):
                one_serp = ujson.load_from_str(serp_data)
                dump_desp = serp_file or "<stdout>"
                logging.info("Dumping serp to %s", dump_desp)
                ujson.dump_to_file(one_serp, serp_file, pretty=True)
                break
            line_num += 1
        else:
            if serp_num is not None:
                raise Exception("Serp number is out of range")
            else:
                raise Exception("Could not find pattern in serpset")


def parse_serpsets(pool, pool_parse_ctx):
    """
    :type pool: Pool
    :type pool_parse_ctx: PoolParseContext
    """
    if not pool.has_valid_serpsets():
        raise Exception("This pool have no valid serpsets (without errors). There is nothing to parse. ")

    serpset_ids = pool.all_serpset_ids()
    logging.info("Parsing fetched %d serpsets.", len(serpset_ids))

    if is_pool_already_parsed(serpset_ids, pool_parse_ctx.parsed_serp_storage):
        logging.info("Pool is already calculated, doing nothing")
        return

    logging.info("Pool cache is incomplete, rebuilding pool. ")

    pool_parse_ctx.raw_serp_storage.display_contents(serpset_ids)

    mgr = multiprocessing.Manager()
    query_id_map = mgr.dict()

    args = []
    lock = mgr.Lock()
    for serpset_id in serpset_ids:
        serpset_parser_ctx = SerpsetParseContext(pool_parse_ctx, serpset_id)
        one_arg = (serpset_parser_ctx, query_id_map, lock)
        args.append(one_arg)

    logging.info("Starting parallel serpset parsing for %d jobs", len(args))
    mp_results = umisc.par_imap_unordered(mp_parse_one_serpset, args, pool_parse_ctx.threads)
    for (arg, res) in mp_results:
        if isinstance(res, BaseException):
            logging.error("Exception in parse_serpsets: %s", res)
            raise res

    logging.info("Serpsets parsing done.")
