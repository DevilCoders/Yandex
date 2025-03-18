# -*- coding: utf-8 -*-
import itertools
import logging
import time

import yt.wrapper as yt

import adminka.ab_cache
import adminka.pool_enrich_services as enrichserv
import mstand_utils.args_helpers as mstand_uargs
import pytlib.yt_io_helpers as yt_io
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.six_helpers as usix
import yaqutils.url_helpers as uurl

from experiment_pool import ExperimentForCalculation
from mstand_enums.mstand_online_enums import ServiceEnum
from session_metric.metric_calculator import MetricSources
from session_yt.versions_yt import MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE

ENRICH_KEY_PREFIX = "__ENRICH_KEY__"

MAX_KEY_SIZE_EST = 15000  # trying to avoid "Key weight is too large" error (default limit is 16K)


class SqueezeEnricherInputMapper(object):
    def __init__(self, input_keys, input_values, squeeze_keys, squeeze_values):
        """
        :type input_keys: list[str]
        :type input_values: list[str]
        :type squeeze_keys: list[str]
        :type squeeze_values: list[str]
        """
        self.key_paths = [x.split(".") for x in input_keys]
        self.value_paths = [x.split(".") for x in input_values]
        self.key_names = squeeze_keys
        self.value_names = squeeze_values
        logging.debug("SqueezeEnricherInputMapper:\n%s\n%s\n%s\n%s",
                      self.key_paths, self.value_paths, self.key_names, self.value_names)

    def __call__(self, row):
        """
        :type row: dict
        :rtype: __generator[dict]
        """
        result_row = {}
        row_size_est = 0

        for name, path in zip(self.key_names, self.key_paths):
            value = self.complex_get(path, row)
            if value is None:
                # skip row if key is not full
                return
            if name == "url":
                value = uurl.normalize_url(value)
            if isinstance(value, str):
                row_size_est += len(value)
            result_row[ENRICH_KEY_PREFIX + name] = value

        if row_size_est > MAX_KEY_SIZE_EST:
            # trying to avoid "Key weight is too large" error
            return

        for name, path in zip(self.value_names, self.value_paths):
            result_row[name] = self.complex_get(path, row)

        yield result_row

    @staticmethod
    def complex_get(path, row):
        value = row
        for part in path:
            if part not in value:
                return None
            value = value[part]
        return value


FIELDS_INHERITED_FROM_REQUEST = {"request_params", "wizard", "wiz_show", "userregion", "userpersonalization", "referer",
                                 "queryregion", "query", "hasmisspell", "dir_show", "correctedquery", "clid", "browser"}


class SqueezeEnricherPrepareReducer(object):
    def __init__(self, squeeze_keys):
        """
        :type squeeze_keys: list[str]
        """
        self.squeeze_keys = squeeze_keys

    def __call__(self, keys, rows):
        saved_rows = []
        requests = {}

        for row in rows:
            if row.get("type") == "request":
                reqid = row.get("reqid")
                if reqid:
                    requests[reqid] = row
            saved_rows.append(row)

        for row in saved_rows:
            row_size_est = 0
            for name in self.squeeze_keys:
                value = row.get(name)
                if row.get("type") != "request" and name in FIELDS_INHERITED_FROM_REQUEST:
                    assert value is None
                    reqid = row.get("reqid")
                    request = requests.get(reqid, {})
                    value = request.get(name)
                if name == "url":
                    value = uurl.normalize_url(value)
                if isinstance(value, str):
                    row_size_est += len(value)
                row[ENRICH_KEY_PREFIX + name] = value
            if row_size_est > MAX_KEY_SIZE_EST:
                # trying to avoid "Key weight is too large" error
                # add yuid to value to avoid big jobs in reduce
                no_value = "__NO_VALUE__" + row["yuid"]
                for name in self.squeeze_keys:
                    value = row.get(ENRICH_KEY_PREFIX + name)
                    if isinstance(value, str):
                        row[ENRICH_KEY_PREFIX + name] = no_value
            yield row


@yt.with_context
class SqueezeEnricherMainReducer(object):
    def __init__(self, value_names, enrich_keys):
        """
        :type value_names: list[str]
        :type enrich_keys: list[str]
        """
        self.value_names = value_names
        self.enrich_keys = enrich_keys

    def __call__(self, keys, rows, context):
        row_with_data = None
        squeeze_rows_was_seen = False
        for row in rows:
            index = context.table_index
            if index == 0:
                if row_with_data is None:
                    row_with_data = row
                else:
                    logging.debug("multiple rows with same key in source table: %s", row_with_data)
                assert not squeeze_rows_was_seen, "(internal bug) source row after squeeze row"
            else:
                squeeze_rows_was_seen = True
                if row_with_data is not None:
                    for name in self.value_names:
                        row[name] = row_with_data[name]
                for name in self.enrich_keys:
                    del row[name]
                yield row


def parse_pair_of_fields(raw_string):
    """
    :type raw_string: str
    :rtype: str, str
    """
    parts = raw_string.split("=")
    assert len(parts) == 2
    left, right = parts
    assert left
    assert right
    return left, right


def ensure_list_is_unique(list_to_check):
    assert len(list_to_check) == len(set(list_to_check)), "field names should be unique"


def prepare_input_table(
        input_table,
        input_keys,
        input_values,
        squeeze_path,
        squeeze_keys,
        squeeze_values,
        yt_pool,
        yt_client,
):
    time_start = time.time()
    input_mapper = SqueezeEnricherInputMapper(input_keys, input_values, squeeze_keys, squeeze_values)
    expiration_hour = 1000 * 60 * 60
    expiration_day = expiration_hour * 24
    yt_spec_input_map = yt_io.get_yt_operation_spec(title="SqueezeEnricher: prepare input",
                                                    max_failed_job_count=10,
                                                    data_size_per_job=500 * yt.common.MB,
                                                    data_size_per_map_job=500 * yt.common.MB,
                                                    partition_data_size=500 * yt.common.MB,
                                                    yt_pool=yt_pool,
                                                    use_porto_layer=False)
    yt_spec_sort_input = yt_io.get_yt_operation_spec(title="SqueezeEnricher: sort prepared input",
                                                     yt_pool=yt_pool,
                                                     use_porto_layer=False)
    with yt.Transaction(client=yt_client):
        tmp_input = yt.create_temp_table(
            path=squeeze_path,
            prefix="enrich_squeeze_input_",
            expiration_timeout=expiration_day * 14,
            client=yt_client,
        )
        logging.info("Will prepare input: %s -> %s", input_table, tmp_input)
        yt.run_map(
            input_mapper,
            source_table=input_table,
            destination_table=tmp_input,
            spec=yt_spec_input_map,
            client=yt_client,
        )
        logging.info("Will sort prepared input: %s", tmp_input)
        yt.run_sort(
            source_table=tmp_input,
            sort_by=[ENRICH_KEY_PREFIX + name for name in squeeze_keys],
            spec=yt_spec_sort_input,
            client=yt_client,
        )
    umisc.log_elapsed(time_start, "Preparing input from %s done", input_table)
    return tmp_input


def enrich_all_tables(
        tmp_input,
        squeeze_tables,
        squeeze_keys,
        squeeze_values,
        replace_existing_fields,
        threads,
        yt_pool,
        yt_client,
):
    count = len(squeeze_tables)
    logging.info("Will enrich %d squeeze tables: %s", count, umisc.to_lines(squeeze_tables))

    def worker(sq_table):
        yt_client_clone = yt.client.YtClient(config=yt_client.config)
        enrich_one_table(
            tmp_input=tmp_input,
            sq_table=sq_table,
            squeeze_keys=squeeze_keys,
            squeeze_values=squeeze_values,
            replace_existing_fields=replace_existing_fields,
            yt_pool=yt_pool,
            yt_client=yt_client_clone,
        )

    results_iter = umisc.par_imap_unordered(worker, squeeze_tables, threads, dummy=True)
    for pos, _ in enumerate(results_iter):
        umisc.log_progress("enrich", pos, count)


def enrich_one_table(
        tmp_input,
        sq_table,
        squeeze_keys,
        squeeze_values,
        replace_existing_fields,
        yt_pool,
        yt_client,
):
    time_start = time.time()
    with yt.Transaction(client=yt_client):
        sq_sort_by = ["yuid", "ts", "action_index"]
        sq_schema = yt.get_attribute(sq_table, "schema")
        squeeze_current_fields = set()
        for schema_element in sq_schema:
            if "sort_order" in schema_element:
                assert schema_element["name"] in sq_sort_by
                assert schema_element["sort_order"] == "ascending"
                del schema_element["sort_order"]
            squeeze_current_fields.add(schema_element["name"])
        assert all(key in squeeze_current_fields for key in squeeze_keys), "squeeze table has no key"
        sq_fields_to_set = []
        for new_field in squeeze_values:
            if new_field not in squeeze_current_fields:
                sq_schema.append({
                    "name": new_field,
                    "type": "any",
                })
                sq_fields_to_set.append(new_field)
            elif replace_existing_fields:
                sq_fields_to_set.append(new_field)
        enrich_one_table_impl(
            tmp_input=tmp_input,
            sq_table=sq_table,
            sq_sort_by=sq_sort_by,
            sq_schema=sq_schema,
            sq_fields_to_set=sq_fields_to_set,
            squeeze_keys=squeeze_keys,
            yt_pool=yt_pool,
            yt_client=yt_client,
        )
    umisc.log_elapsed(time_start, "Enriching for %s done", sq_table)


def enrich_one_table_impl(
        tmp_input,
        sq_table,
        sq_sort_by,
        sq_schema,
        sq_fields_to_set,
        squeeze_keys,
        yt_pool,
        yt_client,
):
    if not sq_fields_to_set:
        logging.info("skip %s - all fields exist", sq_table)
        return

    yt_spec_prepare_squeeze = yt_io.get_yt_operation_spec(title="SqueezeEnricher: prepare squeeze",
                                                          max_failed_job_count=10,
                                                          data_size_per_job=500 * yt.common.MB,
                                                          yt_pool=yt_pool,
                                                          use_porto_layer=False)
    yt_spec_sort_input = yt_io.get_yt_operation_spec(title="SqueezeEnricher: sort squeeze",
                                                     yt_pool=yt_pool,
                                                     use_porto_layer=False)
    yt_spec_enrich_squeeze = yt_io.get_yt_operation_spec(title="SqueezeEnricher: enrich squeeze",
                                                         max_failed_job_count=10,
                                                         data_size_per_job=500 * yt.common.MB,
                                                         yt_pool=yt_pool,
                                                         use_porto_layer=False)
    sq_new_attributes = {
        "schema": sq_schema,
    }
    sq_version = yt.get_attribute(sq_table, MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE, client=yt_client)
    enrich_keys = [ENRICH_KEY_PREFIX + name for name in squeeze_keys]
    prepare_reducer = SqueezeEnricherPrepareReducer(squeeze_keys)
    main_reducer = SqueezeEnricherMainReducer(sq_fields_to_set, enrich_keys)
    with yt.TempTable("//tmp", "enrich_squeeze_result_", client=yt_client) as tmp_squeeze:
        logging.info("Will prepare enrich squeeze table: %s -> %s", sq_table, tmp_squeeze)
        yt.run_reduce(
            prepare_reducer,
            source_table=sq_table,
            destination_table=tmp_squeeze,
            reduce_by=["yuid"],
            spec=yt_spec_prepare_squeeze,
            client=yt_client,
        )
        logging.info("Will resort squeeze table: %s", tmp_squeeze)
        yt.run_sort(
            source_table=tmp_squeeze,
            sort_by=enrich_keys,
            spec=yt_spec_sort_squeeze,
            client=yt_client,
        )
        logging.info("Will enrich squeeze table: %s + %s", tmp_input, tmp_squeeze)
        yt.run_reduce(
            main_reducer,
            source_table=[tmp_input, tmp_squeeze],
            destination_table=yt.TablePath(tmp_squeeze, attributes=sq_new_attributes),
            reduce_by=enrich_keys,
            spec=yt_spec_enrich_squeeze,
            client=yt_client,
        )
        logging.info("Will sort squeeze table: %s", tmp_squeeze)
        yt.run_sort(
            source_table=tmp_squeeze,
            sort_by=sq_sort_by,
            spec=yt_spec_sort_squeeze,
            client=yt_client,
        )
        yt.set_attribute(
            path=tmp_squeeze,
            attribute=MSTAND_SQUEEZE_VERSIONS_ATTRIBUTE,
            value=sq_version,
            client=yt_client,
        )
        yt.move(
            source_path=tmp_squeeze,
            destination_path=sq_table,
            client=yt_client,
            force=True,
            preserve_account=False,
            preserve_expiration_time=False,
        )


class SqueezeEnricher(object):
    def __init__(
            self,
            squeeze_path,
            session,
            history=None,
            future=None,
            use_filters=False,
    ):
        """
        :type squeeze_path: str
        :type history: int | None
        :type future: int | None
        :type use_filters: bool
        """
        self.squeeze_path = squeeze_path

        self.history = history
        if self.history:
            assert self.history > 0
        self.future = future
        if self.future:
            assert self.future > 0

        self.session = session

        self.use_filters = use_filters

    @staticmethod
    def from_cli_args(cli_args, session):
        return SqueezeEnricher(
            squeeze_path=cli_args.squeeze_path,
            history=cli_args.history,
            future=cli_args.future,
            use_filters=cli_args.use_filters,
            session=session,
        )

    @staticmethod
    def add_cli_args(parser):
        mstand_uargs.add_history(parser)
        mstand_uargs.add_squeeze_path(parser, "//home/mstand/squeeze")
        mstand_uargs.add_use_filters_flag(parser)

    def enrich_observations(
            self,
            observations,
            services_with_meta,
            enrich_keys_raw,
            enrich_values_raw,
            replace_existing_fields,
            input_table,
            threads,
            yt_pool,
            yt_config,
    ):
        """
        :type observations: list[Observation]
        :type services_with_meta: list[str]
        :type enrich_keys_raw: list[str]
        :type enrich_values_raw: list[str]
        :type replace_existing_fields: bool
        :type input_table: str
        :type threads: int
        :type yt_pool: str
        :type yt_config: dict[str]
        """
        enrich_keys = [parse_pair_of_fields(x) for x in enrich_keys_raw]
        enrich_values = [parse_pair_of_fields(x) for x in enrich_values_raw]

        squeeze_keys = [key_squeeze for key_squeeze, _ in enrich_keys]
        input_keys = [key_input for _, key_input in enrich_keys]
        squeeze_values = [value_squeeze for value_squeeze, _ in enrich_values]
        input_values = [value_input for _, value_input in enrich_values]
        ensure_list_is_unique(input_keys + input_values)
        ensure_list_is_unique(squeeze_keys + squeeze_values)

        assert len(input_keys) == len(squeeze_keys)
        assert len(input_values) == len(squeeze_values)

        squeeze_tables = self._list_squeeze_tables(observations, services_with_meta, yt_config)

        tmp_input = None
        yt_client = yt.client.YtClient(config=yt_config)
        try:
            tmp_input = prepare_input_table(
                input_table=input_table,
                input_keys=input_keys,
                input_values=input_values,
                squeeze_path=self.squeeze_path,
                squeeze_keys=squeeze_keys,
                squeeze_values=squeeze_values,
                yt_pool=yt_pool,
                yt_client=yt_client,
            )
            enrich_all_tables(
                tmp_input=tmp_input,
                squeeze_tables=squeeze_tables,
                squeeze_keys=squeeze_keys,
                squeeze_values=squeeze_values,
                replace_existing_fields=replace_existing_fields,
                threads=threads,
                yt_pool=yt_pool,
                yt_client=yt_client,
            )
        finally:
            if tmp_input:
                logging.info("Will clear prepared input: %s", tmp_input)
                yt.remove(tmp_input, client=yt_client)

    def _list_squeeze_tables(self, observations, services_with_meta, yt_config):
        experiments_by_obs = []
        for obs in observations:
            obs_exps = ExperimentForCalculation.from_observation(obs)
            if obs_exps:
                experiments_by_obs.append(sorted(obs_exps))
        services = enrichserv.extend_services_from_observation(observations,
                                                               self.session,
                                                               services_with_meta,
                                                               self.use_filters)
        sources = {}
        for exps in experiments_by_obs:
            for exp in exps:
                if exp not in sources:
                    sources[exp] = MetricSources.from_experiment(
                        experiment=exp.experiment,
                        observation=exp.observation,
                        services=services,
                        squeeze_path=self.squeeze_path,
                        history=self.history,
                        future=self.future,
                    )
        all_tables = sorted(set(itertools.chain.from_iterable(s.all_tables() for s in sources.itervalues())))
        logging.info("Will check if %d tables exist: %s", len(all_tables), umisc.to_lines(all_tables))
        yt_client = yt.client.YtClient(config=yt_config)
        bad_tables = [path for path in all_tables
                      if not yt.exists(path, client=yt_client) or yt.is_empty(path, client=yt_client)]
        if bad_tables:
            raise Exception("{} tables do not exist: {}".format(len(bad_tables), umisc.to_lines(bad_tables)))
        return all_tables


def add_enrich_squeeze_arguments(parser):
    uargs.add_verbosity(parser)
    mstand_uargs.add_list_of_online_services(parser, possible=ServiceEnum.get_all_services())
    mstand_uargs.add_buckets(parser)
    mstand_uargs.add_yt(parser)
    uargs.add_threads(parser, default=8)

    parser.add_argument(
        "--enrich-table-path",
        required=True,
        help="path to YT table",
    )
    parser.add_argument(
        "--enrich-keys",
        required=True,
        nargs="+",
        help="List of squeeze-field=enrich-field for matching. "
             "You can use dots to access complex objects in enrich-table. Example: "
             "--enrich-keys correctedquery=input.query userregion=input.region_id url=input.url)",
    )
    parser.add_argument(
        "--enrich-values",
        required=True,
        nargs="+",
        help="List of squeeze-field=enrich-field for getting values. "
             "You can use dots to access complex objects in enrich-table. Example: "
             "--enrich-values yangrelevance5=result.relevance)",
    )
    uargs.add_boolean_argument(
        parser,
        name="--replace-existing-fields",
        help_message="do not skip when some of fields exist in squeeze",
    )
