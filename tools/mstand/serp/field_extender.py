import functools
import logging
import os
import time
import traceback

from collections import defaultdict

import mstand_utils.mstand_module_helpers as mstand_umodule

import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.json_helpers as ujson
import yaqutils.six_helpers as usix

from serp import FieldExtenderContext
from serp import FieldExtenderKey
from serp import ExtendSettings  # noqa
from serp import QueryKey

from serp import SerpQueryInfo  # noqa
from serp import SerpMarkupInfo  # noqa
from serp import SerpUrlsInfo  # noqa

from serp import SerpSetFileHandler
from serp import SerpSetReader
from serp import JsonLinesWriter

from yaqutils import MainUtilsException

from serp import ExtendParamsForAPI


class ParseError(MainUtilsException):
    pass


SERP_TOP_RANGES = (5, 10, 15, 30, "all")


def create_custom_extender_from_cli(cli_args):
    if not cli_args.module_name or not cli_args.class_name:
        logging.info("No module or class for extender, using internal serpset extension.")
        return None

    logging.info("Module/class specified, creating custom scale extender.")

    scale_extender = mstand_umodule.create_user_object(
        module_name=cli_args.module_name,
        class_name=cli_args.class_name,
        source=cli_args.source,
        kwargs=cli_args.user_kwargs)

    if not hasattr(scale_extender, "extend"):
        raise Exception("Custom extender class should have 'extend' method. ")

    return scale_extender


def serp_extend_main(pool, enrichment_file, extend_settings, parsed_serp_storage, extender, threads=1):
    start_time = time.time()
    logging.info("Extracting new field values from %s", enrichment_file)

    # using of multiprocessing.Manager.dict slows processing by 6-8 times.

    threads = estimate_enrichment_thread_count(threads, enrichment_file)

    field_values = load_enrichment_tsv_file(enrichment_file, extend_settings)
    logging.info("Extracted new field values count: %s", len(field_values))

    logging.info("Starting serpset extension")
    ctx = FieldExtenderContext(parsed_serp_storage=parsed_serp_storage,
                               field_values=field_values,
                               extend_settings=extend_settings,
                               extender=extender,
                               threads=threads)

    extend_serpsets(pool.all_serpset_ids(), ctx=ctx)
    umisc.log_elapsed(start_time, "serpsets extended")


def estimate_enrichment_thread_count(threads, input_file):
    size_threads_limit = {
        128 * ufile.MB: 2,
        256 * ufile.MB: 1,
    }

    # if enrichment file is large (~500M and above), it's copied multiple times inside multiprocessing,
    # vanishing all 'positive' effect of parallel enrichment and even making it much slower.

    # to reduce this negative impact, we use large chunksize (reduces amount of internal data copying).
    # TODO:
    # more elegant solution:
    # use own MP-like pool which does not 'refill' worker with data on every single job,
    # just create queue of serpsets and 2-3 'static' worker threads that read field_values dict
    # independently.

    if ufile.use_std_stream(input_file):
        optimal_threads = 1
    else:
        file_size = os.path.getsize(input_file)
        for size_threshold in sorted(size_threads_limit.keys(), reverse=True):
            if file_size >= size_threshold:
                max_threads = size_threads_limit[size_threshold]
                optimal_threads = min(max_threads, threads)
                break
        else:
            optimal_threads = 4

    logging.info("Optimal threads number: %d", optimal_threads)
    return optimal_threads


def parse_enrichment_tsv_file(field_values, tsv_fd, key_settings):
    """
    :type field_values:
    :type tsv_fd:
    :type key_settings: ExtendSettings
    :rtype: None
    """

    # TSV file format: query, region, url, [pos,] <json-serialized-value>
    # currently, device, uid and country are not used here.

    expected_len = 5 if key_settings.with_position else 4

    for index, line in enumerate(tsv_fd):
        line_num = index + 1
        row = line.rstrip().split("\t")
        act_len = len(row)
        if act_len != expected_len:
            raise ParseError("Wrong field count in TSV on line {}: expected {}, got {}".format(line_num,
                                                                                               expected_len,
                                                                                               act_len))

        if line_num % 50000 == 1:
            logging.info("Processed %d rows", line_num)
        extend_key, custom_value = parse_one_enrichment_line(row, key_settings)
        logging.debug("Loaded pair: key %s, value %s", extend_key, custom_value)
        try:
            field_values[extend_key] = ujson.load_from_str(custom_value)
        except Exception as exc:
            logging.error("Cannot parse JSON '{}' on line {}: {}".format(custom_value, line_num, exc))
            raise


def parse_one_enrichment_line(row, key_settings):
    index = 0
    query_text = row[index]

    index += 1
    region = int(row[index])

    index += 1
    url = row[index]

    if key_settings.with_position:
        index += 1
        pos = int(row[index])
    else:
        pos = None

    index += 1
    custom_value = row[index]

    # enrichment is now done only by query_text and query_region parts of QueryKey.

    query_key = QueryKey(query_text=query_text, query_region=region)
    if key_settings.query_mode:
        if url != "-":
            raise Exception("URL should be '-' in query-only mode.")
    extend_key = FieldExtenderKey(query_key, url, pos)
    return extend_key, custom_value


def load_enrichment_tsv_file(tsv_file, extend_settings):
    """
    :type tsv_file: str
    :type extend_settings: ExtendSettings
    :rtype: dict
    """
    start_time = time.time()

    logging.info("Parsing enrichment file %s, %s", tsv_file, extend_settings)

    field_values = {}
    logging.info("Reading enrichment file from file %s", tsv_file)
    with ufile.fopen_read(tsv_file) as tsv_fd:
        parse_enrichment_tsv_file(field_values, tsv_fd, extend_settings)
    umisc.log_elapsed(start_time, "enrichment file parsing")
    return field_values


def extend_serpsets(serpset_ids, ctx):
    """
    :type serpset_ids: set[str]
    :type ctx: FieldExtenderContext
    :rtype: None
    """
    logging.info("Extending %d serpsets", len(serpset_ids))

    # arg_list should be 'small' object (it's serialized inside MP)
    arg_list = []
    for serpset_id in serpset_ids:
        arg = serpset_id
        arg_list.append(arg)

    job_number = len(arg_list)
    logging.info("Will extend %d serpsets", job_number)

    worker_with_ctx = functools.partial(mp_extend_one_serpset_wrapper, ctx)
    # chunksize=None means 'auto' chunk size (which yields ~10 for pool of ~100 jobs)
    pool_results = umisc.par_imap_unordered(worker_with_ctx, arg_list, ctx.threads, chunksize=None)

    start_time = time.time()

    for index, (arg, res) in enumerate(pool_results):
        if isinstance(res, BaseException):
            logging.error(u"Error occured, stopping pool processing: %s", res)
            raise res
        umisc.log_progress("serpset extension", index=index, total=job_number)

    umisc.log_elapsed(start_time, "serpsets extension")


def mp_extend_one_serpset_wrapper(ctx, arg):
    """
    :type ctx: FieldExtenderContext
    :type arg: str
    :rtype: None
    """
    serpset_id = arg

    try:
        extend_and_rewrite_one_serpset(serpset_id, ctx)
        return arg, None
    except BaseException as exc:
        logging.error("Error extending serpset %s: %s, %s", serpset_id, exc, traceback.format_exc())
        return arg, exc


def extend_and_rewrite_one_serpset(serpset_id, ctx):
    """
    :type serpset_id: str
    :type ctx: FieldExtenderContext
    :rtype: None
    """
    parsed_serp_storage = ctx.parsed_serp_storage

    markup_file = parsed_serp_storage.markup_by_serpset(serpset_id)
    queries_file = parsed_serp_storage.queries_by_serpset(serpset_id)
    if not ctx.extend_settings.query_mode:
        urls_file = parsed_serp_storage.urls_by_serpset(serpset_id)
    else:
        urls_file = None

    logging.info("Extending data in serpset %s", serpset_id)

    start_time = time.time()

    extend_one_serpset(serpset_id=serpset_id,
                       queries_file=queries_file,
                       markup_file=markup_file,
                       urls_file=urls_file,
                       field_values=ctx.field_values,
                       extend_settings=ctx.extend_settings,
                       extender=ctx.extender)

    umisc.log_elapsed(start_time, "serpset %s extended", serpset_id)


def extend_one_serpset(serpset_id, queries_file, markup_file, urls_file,
                       field_values, extend_settings, extender=None):
    """
    :type serpset_id: str
    :type queries_file: str
    :type markup_file: str
    :type urls_file: str | None
    :type field_values: dict[FieldExtenderKey]
    :type extend_settings: ExtendSettings
    :type extender:
    :rtype: None
    """

    serpset_file_handler = SerpSetFileHandler(queries_file=queries_file, urls_file=urls_file,
                                              markup_file=markup_file, mode="read")

    new_markup_file = "{}.tmp".format(markup_file)

    with serpset_file_handler:
        with ufile.fopen_write(new_markup_file) as new_markup_fd:
            serpset_reader = SerpSetReader.from_file_handler(serpset_file_handler)
            new_markup_writer = JsonLinesWriter(new_markup_fd, cache_size=100)

            # calculate some stats to make debugging easy
            query_count = 0
            ext_queries = 0
            ext_stats_total = defaultdict(int)

            for parsed_serp in serpset_reader:
                logging.info("got %s, %s, %s", parsed_serp.query_info, parsed_serp.markup_info, parsed_serp.urls_info)
                query_count += 1

                # markup_info is changed in-place
                if extend_settings.query_mode:
                    is_extended, ext_stats = extend_in_query_mode(query_info=parsed_serp.query_info,
                                                                  markup_info=parsed_serp.markup_info,
                                                                  field_values=field_values,
                                                                  extend_settings=extend_settings,
                                                                  extender=extender)
                else:
                    is_extended, ext_stats = extend_in_url_mode(query_info=parsed_serp.query_info,
                                                                markup_info=parsed_serp.markup_info,
                                                                urls_info=parsed_serp.urls_info,
                                                                field_values=field_values, extend_settings=extend_settings,
                                                                extender=extender)
                if is_extended:
                    ext_queries += 1
                    update_top_stats_for_serpset(ext_stats_total, ext_stats)
                new_markup_writer.write_row(parsed_serp.markup_info)

            new_markup_writer.flush()

            log_enrichment_stats(serpset_id, query_count, ext_queries, ext_stats_total, extend_settings)

    logging.info("Updating markup for serpset %s", serpset_id)
    os.rename(new_markup_file, markup_file)


def log_enrichment_stats(serpset_id, total_queries, ext_queries, ext_stats, key_settings):
    """
    :type serpset_id: str
    :type total_queries: int
    :type ext_queries: int
    :type ext_stats: defaultdict[int]
    :type key_settings: ExtendSettings
    :rtype:
    """
    ratio = float(ext_queries * 100) / total_queries if total_queries else 0.0
    logging.info("Extended %d of %d queries for serpset %s (%.2f%%)", ext_queries, total_queries, serpset_id, ratio)

    if not key_settings.query_mode:
        for top in SERP_TOP_RANGES:
            if top not in ext_stats:
                continue
            ext_results_count = ext_stats[top]
            res_per_query = float(ext_results_count) / float(ext_queries) if ext_queries else 0.0
            logging.info("--> extended [top:%3s] %d results in %d queries (avg. %.2f per query) for serpset %s",
                         top, ext_results_count, ext_queries, res_per_query, serpset_id)


def extend_in_query_mode(query_info, markup_info, field_values, extend_settings, extender):
    """
    :type query_info: SerpQueryInfo
    :type markup_info: SerpMarkupInfo
    :type field_values: dict[FieldExtenderKey]
    :type extend_settings: ExtendSettings
    :type extender: callable
    :rtype: tuple[bool, defaultdict[int]]
    """
    is_query_extended = False

    match_key = FieldExtenderKey(query_info.query_key, url="-", pos=None)
    if match_key in field_values:
        is_query_extended = True
        custom_value = field_values[match_key]
        logging.debug("Extending %s by value %r", query_info, custom_value)
        # logging.info("serp_data size = %s", len(markup_info.serp_data))
        extend_scales_dict(scales=markup_info.serp_data,
                           extend_settings=extend_settings,
                           raw_custom_value=custom_value,
                           extender=extender,
                           pos=None,
                           ctx_info=query_info, )

    return is_query_extended, defaultdict(int)


def extend_scales_dict(scales, extend_settings, raw_custom_value, extender, pos, ctx_info):
    """
    :type scales: dict
    :type extend_settings: ExtendSettings
    :type raw_custom_value:
    :type extender:
    :type pos: int | None
    :type ctx_info:
    :rtype:
    """
    do_overwrite = extend_settings.overwrite

    # TODO: make internal extenders acting like custom.

    if extender:
        extend_params = ExtendParamsForAPI(scales=scales, field_name=extend_settings.field_name,
                                           custom_value=raw_custom_value, do_overwrite=do_overwrite,
                                           pos=pos)
        extender.extend(extend_params)
    elif not extend_settings.flat_mode:

        ext_field_name = extend_settings.field_name

        if not do_overwrite and ext_field_name in scales:
            raise Exception("Field '{}' exists (no-overwrite, non-flat). ctx: {}".format(ext_field_name, ctx_info))

        scales[ext_field_name] = raw_custom_value
    else:
        if not isinstance(raw_custom_value, dict):
            raise Exception(u"Value '{}' is not a dict, can't use flat mode. ctx: {}".format(raw_custom_value, ctx_info))

        for custom_key, custom_value in usix.iteritems(raw_custom_value):
            ext_field_name = "{}{}".format(extend_settings.field_name, custom_key)

            if not do_overwrite and ext_field_name in scales:
                raise Exception("Field '{}' already exists (no-overwrite, flat mode). ctx: {}".format(ext_field_name,
                                                                                                      ctx_info))
            scales[ext_field_name] = custom_value


def update_top_stats_for_serpset(ext_stats_total, ext_stats):
    """
    :type ext_stats_total: defaultdict[int]
    :type ext_stats: defaultdict[int]
    :rtype:
    """
    for top, ext_results_count in usix.iteritems(ext_stats):
        ext_stats_total[top] += ext_stats[top]


def update_top_stats_for_query(ext_stats, tops, index):
    """
    :type ext_stats: defaultdict[int]
    :type tops: tuple
    :type index: int
    :rtype: None
    """
    for top in tops:
        if top == "all":
            ext_stats[top] += 1
        else:
            if index < top:
                ext_stats[top] += 1


def extend_in_url_mode(query_info, markup_info, urls_info, field_values, extend_settings, extender):
    """
    :type query_info: SerpQueryInfo
    :type markup_info: SerpMarkupInfo
    :type urls_info: SerpUrlsInfo
    :type field_values: dict[FieldExtenderKey]
    :type extend_settings: ExtendSettings
    :type extender:
    :rtype:
    """
    is_query_extended = False
    # MSTAND-879
    ext_stats = defaultdict(int)

    for rindex, res_markup in enumerate(markup_info.res_markups):
        res_url = urls_info.res_urls[rindex]
        match_url = res_url.url
        match_pos = res_markup.pos if extend_settings.with_position else None

        match_key = FieldExtenderKey(query_info.query_key, url=match_url, pos=match_pos)
        # logging.debug("Checking key: %s", match_key)
        if match_key in field_values:
            is_query_extended = True
            custom_value = field_values[match_key]
            logging.debug("Extending key %s by value %s", match_key, custom_value)
            extend_scales_dict(scales=res_markup.scales,
                               extend_settings=extend_settings,
                               raw_custom_value=custom_value,
                               extender=extender,
                               ctx_info=query_info,
                               pos=match_pos)

            update_top_stats_for_query(ext_stats, SERP_TOP_RANGES, rindex)

    return is_query_extended, ext_stats
