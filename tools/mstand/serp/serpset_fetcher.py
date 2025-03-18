import logging
import time
import traceback
import os
from typing import List
from typing import Union
from typing import Tuple
from typing import Set
from typing import Optional

import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc

import serp.serpset_converter as ss_conv
import serp.serpset_fetcher_single as sf_single

import experiment_pool.pool_helpers as phelp

from experiment_pool import ExpErrorType
from experiment_pool import Pool  # noqa

import serp.serp_helpers as shelp

from serp import RawSerpDataStorage  # noqa
from serp import SerpFetchParams  # noqa


class ConvContext(object):
    def __init__(self, raw_serp_storage: RawSerpDataStorage, serpset_id: str,
                 do_unpack: bool, use_external_convertor: bool = True):
        self.raw_serp_storage = raw_serp_storage
        self.serpset_id = serpset_id
        self.do_unpack = do_unpack
        self.use_external_convertor = use_external_convertor


class FetchContext(object):
    def __init__(self, serpset_id, raw_serp_storage, fetch_params):
        self.serpset_id = serpset_id
        self.raw_serp_storage = raw_serp_storage
        self.fetch_params = fetch_params
        self.is_downloaded = None


def mp_fetch_one_serpset_worker(fetch_context: FetchContext):
    try:
        raw_serp_storage = fetch_context.raw_serp_storage
        fetch_params = fetch_context.fetch_params
        serpset_id = fetch_context.serpset_id

        if raw_serp_storage.should_download_serpset(serpset_id):
            fetch_context.is_downloaded = sf_single.fetch_serpset(serpset_id, raw_serp_storage, fetch_params)
        else:
            fetch_context.is_downloaded = True
        return fetch_context
    except BaseException as exc:
        backtrace = traceback.format_exc()
        logging.error("Exception in mp_fetch_one_serpset_worker: %s, details: %s", exc, backtrace)
        return exc


def fetch_serpsets(pool: Pool, raw_serp_storage: RawSerpDataStorage, fetch_params: SerpFetchParams):
    serpset_ids = pool.all_serpset_ids()

    logging.info('Started fetching %d serpsets', len(serpset_ids))

    ufile.make_dirs(raw_serp_storage.raw_serpset_dir())

    serps_total = 0
    broken_serpset_ids = set()

    mp_fetch_args = [FetchContext(serpset_id, raw_serp_storage, fetch_params) for serpset_id in serpset_ids]
    fetch_results = umisc.par_imap_unordered(mp_fetch_one_serpset_worker, mp_fetch_args, max_threads=fetch_params.fetch_threads)
    for index, res in enumerate(fetch_results):
        if isinstance(res, BaseException):
            raise res
        elif isinstance(res, FetchContext):
            if not res.is_downloaded:
                broken_serpset_ids.add(res.serpset_id)
        else:
            raise Exception("BUG: Bad return value of mp_fetch_one_serpset_worker method")
        logging.info("Serpset fetching: %d of %d done", index + 1, len(serpset_ids))

    serps_broken = len(broken_serpset_ids)
    serps_succeeded = serps_total - serps_broken
    logging.info("Serpsets fetching done, succeeded: %d, broken: %d", serps_succeeded, serps_broken)
    if serps_broken:
        broken_list = ", ".join(broken_serpset_ids)
        logging.warning("Pool has %s broken serpsets: %s", serps_broken, broken_list)
    return broken_serpset_ids


def mp_convert_one_serpset_worker(conv_context: ConvContext) -> Tuple[ConvContext, Union[BaseException, None]]:
    serpset_id = conv_context.serpset_id
    do_unpack = conv_context.do_unpack
    try:
        raw_serp_storage = conv_context.raw_serp_storage
        if raw_serp_storage.should_convert_serpset(serpset_id):
            if do_unpack:
                # internal input
                raw_file = raw_serp_storage.raw_gz_serpset_by_id(serpset_id)
            else:
                # mc input
                raw_file = raw_serp_storage.raw_json_serpset_by_id(serpset_id)

            jsonlines_file = raw_serp_storage.jsonlines_serpset_by_id(serpset_id)

            # raw file is removed by convertor.
            ss_conv.convert_serpset_to_jsonlines(serpset_id=serpset_id, raw_file=raw_file, jsonlines_file=jsonlines_file,
                                                 do_unpack=do_unpack, remove_original=True,
                                                 use_external_convertor=conv_context.use_external_convertor)

        return conv_context, None
    except BaseException as exc:
        logging.info("Exception in mp_convert_one_serpset_worker on serpset %s: %s\n%s",
                     serpset_id, exc, traceback.format_exc())
        return conv_context, exc


def convert_serpsets_to_jsonlines(pool: Pool, raw_serp_storage: RawSerpDataStorage, max_threads: int, do_unpack: bool,
                                  use_external_convertor: bool = True):
    start_time = time.time()
    serpset_ids = pool.all_serpset_ids()
    logging.info('Started converting %d serpsets', len(serpset_ids))

    ufile.make_dirs(raw_serp_storage.raw_serpset_dir())

    conv_mp_args = [ConvContext(raw_serp_storage, serpset_id, do_unpack, use_external_convertor=use_external_convertor)
                    for serpset_id in serpset_ids]

    logging.info("Starting parallel serpset conversion")
    mp_results = umisc.par_imap_unordered(mp_convert_one_serpset_worker, conv_mp_args, max_threads=max_threads)
    for index, (conv_context, res) in enumerate(mp_results):
        if isinstance(res, BaseException):
            logging.error("Error in convert serpsets: %s", res)
            raise res
        umisc.log_progress("serpset conversion", index=index, total=len(conv_mp_args))

    umisc.log_elapsed(start_time, "all serpsets conversion")
    logging.info("Serpsets conversion done")


def fill_fetch_errors(pool: Pool, broken_serpset_ids: Set[str]):
    logging.info("Filling broken serpset information")
    for obs in pool.observations:
        for exp in obs.all_experiments():
            if exp.serpset_id is None:
                continue
            if exp.serpset_id in broken_serpset_ids:
                exp.add_error(ExpErrorType.SERP_FETCH)


def fetch_serpsets_main(pool: Pool, fetch_params: SerpFetchParams, raw_serp_storage: RawSerpDataStorage):
    start_time = time.time()
    broken_serpset_ids = fetch_serpsets(pool, raw_serp_storage, fetch_params)
    fill_fetch_errors(pool, broken_serpset_ids)

    pool_size = len(pool.all_serpset_ids(include_errors=True))
    if not pool_size:
        raise Exception("No serpset IDs in pool. There is nothing to fetch.")
    if len(broken_serpset_ids) == pool_size:
        raise Exception("All {} serpsets in pool are broken. There is nothing to save. ".format(pool_size))

    convert_serpsets_to_jsonlines(pool, raw_serp_storage, max_threads=fetch_params.convert_threads,
                                  do_unpack=True, use_external_convertor=fetch_params.use_external_convertor)

    umisc.log_elapsed(start_time, "serpset fetch/convert")

    logging.info("Saving pool to cache as %s", raw_serp_storage.pool_path())
    phelp.dump_pool(pool, raw_serp_storage.pool_path())


def is_raw_serpset_file(file_path: str) -> bool:
    file_name = os.path.basename(file_path)
    if file_name.startswith("meta_"):
        logging.info("skipping meta-info file %s", file_path)
        return False
    if file_name.startswith("serpset_"):
        logging.info("skipping converted serpset file %s", file_path)
        return False
    return True


def list_and_unpack_mc_serpsets(serpset_dir: str, max_threads: int) -> List[str]:
    logging.info("listing serpset dir %s", serpset_dir)
    arc_files = ufile.list_files_in_dir(serpset_dir)
    # leave raw serpset files only.
    # archive (and directory where it's unpacked) could contain other files,
    # e.g. metafiles or cached converted serpsets from previous manual runs.

    raw_serpset_files = [file_name for file_name in arc_files
                         if is_raw_serpset_file(file_name)]

    serpset_ids = set([])

    unpack_mp_args = [(file_name, serpset_dir) for file_name in raw_serpset_files]

    mp_results = umisc.par_imap_unordered(mp_unpack_one_serpset_worker, unpack_mp_args, max_threads=max_threads)
    for index, res in enumerate(mp_results):
        if isinstance(res, BaseException):
            logging.error("Error in serpsets unpacking: %s", res)
            raise res
        serpset_id = res
        if not serpset_id:
            continue
        serpset_ids.add(serpset_id)
        umisc.log_progress("serpset unpacking", index=index, total=len(unpack_mp_args))

    logging.info("%d serpsets extracted from archive", len(serpset_ids))
    return list(sorted(serpset_ids))


def mp_unpack_one_serpset_worker(arg):
    file_name, serset_dir = arg
    try:
        return unpack_one_serpset(file_name, serset_dir)
    except BaseException as exc:
        logging.error("Cannot unpack one serpset: %s, details: ", exc)
        return exc


def unpack_one_serpset(file_name: str, serpset_dir: str):
    serpset_id, ext = os.path.splitext(file_name)
    # support new export mode: separate .json.gz in tar (MSTAND-1302)
    if ext == ".gz":
        logging.info("serpset %s is gzip-compressed, unpacking", file_name)
        serpset_gz_path = os.path.join(serpset_dir, file_name)
        ufile.gunzip_file(serpset_gz_path)
        serpset_id, ext = os.path.splitext(serpset_id)

    if ext not in {".json", ".jsonl"}:
        logging.warning("Strange file in archive (extension is not .json/jsonl (raw or gzipped): '%s'", file_name)
        return None

    if not serpset_id.isdigit():
        logging.warning("Strange file in archive (name is not 'digital'): '%s'", file_name)
        return None

    return serpset_id


def unpack_mc_serpsets(pool: Pool, mc_serpsets_tar: str,
                       raw_serp_storage: RawSerpDataStorage, max_threads: int) -> List[str]:
    logging.info("Unpacking serpsets from mc from file %s", mc_serpsets_tar)

    assert mc_serpsets_tar
    assert max_threads, "unpack_mc_serpsets() max_threads should be > 0"

    dest_dir = raw_serp_storage.raw_serpset_dir()
    ufile.untar_directory(mc_serpsets_tar, dest_dir=dest_dir)

    logging.info("unpacking serpsets tar archive")
    tar_serpset_ids = list_and_unpack_mc_serpsets(dest_dir, max_threads=max_threads)
    if not tar_serpset_ids:
        hint_msg = "Ensure that your serpsets are in the root of the archive (not nested in directory, etc)."
        raise Exception("Your archive has no serpsets named '<serpset-id>.json'. {}".format(hint_msg))

    if pool is None:
        logging.info("getting pool from serpsets tar archive")
        serpset_ids = tar_serpset_ids
    else:
        logging.info("getting serpset list from pool")
        serpset_ids = pool.all_serpset_ids()
        # MSTAND-1728: take serpsets from pool if it's specified
        # But warn if contents are different
        if set(serpset_ids) != set(tar_serpset_ids):
            logging.warning("Pool and serpset tar archive have different serpsets")

    logging.info("Renaming input serpset files")
    for serpset_id in serpset_ids:
        json_src_name = "{}.json".format(serpset_id)
        json_src_path = os.path.join(dest_dir, json_src_name)

        jsonlines_src_name = "{}.jsonl".format(serpset_id)
        jsonlines_src_path = os.path.join(dest_dir, jsonlines_src_name)
        if ufile.is_file_or_link(json_src_path):
            os.rename(json_src_path, raw_serp_storage.raw_json_serpset_by_id(serpset_id))
        elif ufile.is_file_or_link(jsonlines_src_path):
            os.rename(jsonlines_src_path, raw_serp_storage.jsonlines_serpset_by_id(serpset_id))
        else:
            raise Exception(f"Serpset {serpset_id} not found after archive unpacking. Expected {json_src_path} or {jsonlines_src_path}")
    logging.info("Serpsets from Metrics unpacked OK")
    return serpset_ids


def filter_serpset_ids(serpset_ids, serpset_id_filter) -> List[str]:
    if isinstance(serpset_id_filter, str):
        serpset_id_filter = [serpset_id_filter]

    filtered_serpset_ids = []
    for serpset_id in serpset_ids:
        if serpset_id not in serpset_id_filter:
            continue
        filtered_serpset_ids.append(serpset_id)
    return filtered_serpset_ids


def unpack_mc_serpsets_main(pool: Optional[Pool], mc_serpsets_tar: str, raw_serp_storage: RawSerpDataStorage,
                            convert_threads: int, unpack_threads: int,
                            use_external_convertor: bool = True, serpset_id_filter: Optional[List[str]] = None) -> Pool:
    start_time = time.time()

    serpset_ids = unpack_mc_serpsets(pool, mc_serpsets_tar=mc_serpsets_tar, raw_serp_storage=raw_serp_storage,
                                     max_threads=unpack_threads)

    if serpset_id_filter:
        logging.warning("serpset ID filter specified: %s", serpset_id_filter)
        serpset_ids = filter_serpset_ids(serpset_ids, serpset_id_filter)

    if pool is None:
        pool = shelp.build_mc_pool(serpset_ids)

    convert_serpsets_to_jsonlines(pool, raw_serp_storage, max_threads=convert_threads,
                                  do_unpack=False, use_external_convertor=use_external_convertor)

    umisc.log_elapsed(start_time, "serpset unpack/convert")

    logging.info("Saving pool to cache as %s", raw_serp_storage.pool_path())
    phelp.dump_pool(pool, raw_serp_storage.pool_path())
    return pool
