import datetime as dt
import logging

from typing import Set

from nile.api.v1 import (
    clusters,
    Record,
    files as nfi,
)

import pytlib.yt_io_helpers as yt_io
import yt.wrapper as yt

import mstand_utils.mstand_tables as mstand_tables
import mstand_utils.yt_helpers as mstand_uyt
import session_metric.metric_calculator as m_c

from experiment_pool import Experiment
from experiment_pool import Observation

from mstand_enums.mstand_online_enums import ServiceEnum, ServiceSourceEnum
from mstand_utils.yt_options_struct import TableBounds
from mstand_utils.yt_options_struct import YtJobOptions
from session_squeezer.experiment_for_squeeze import ExperimentForSqueeze
from session_squeezer.squeezer import UserSessionsSqueezer

from user_plugins import PluginBatch
from user_plugins import PluginContainer
from user_plugins import PluginKwargs
from user_plugins import PluginSource

from yaqlibenums import YtOperationTypeEnum


class SurplusCalculator:
    def __init__(self,
                 squeezer: UserSessionsSqueezer,
                 metric_container: PluginContainer):
        self.squeezer = squeezer
        self.metric_container = metric_container

    def __call__(self, sessions):
        for key, container in sessions:
            actions = []
            experiment = None
            observation = None
            for exp, action in self.squeezer.squeeze_nile_session(key, container):
                experiment = exp.experiment
                observation = exp.observation
                actions.append(action)

            if experiment and observation and actions:
                actions.sort(key=lambda x: (x["ts"], x["action_index"]))
                metrics = m_c.calc_metric_for_user_impl(
                    metric_container=self.metric_container,
                    yuid=key,
                    rows=actions,
                    experiment=experiment,
                    observation=observation,
                    use_buckets=False,
                )
                if metrics:
                    result = dict(yuid=key)
                    for value in metrics.values():
                        result.update(value)
                    yield Record.from_dict(result)


def create_experiments_for_squeeze() -> Set[ExperimentForSqueeze]:
    experiment = Experiment(testid="0")
    observation = Observation(obs_id=None, dates=None, control=experiment)
    return {
        ExperimentForSqueeze(experiment, observation, srv)
        for srv in ServiceEnum.WEB_ALL
    }


def create_metric_container() -> PluginContainer:
    return PluginContainer(
        plugin_batch=PluginBatch(
            plugin_sources=[
                PluginSource(
                    module_name="surplus_search.v8.surplus",
                    class_name="SurplusData",
                    kwargs_list=[
                        PluginKwargs(kwargs={"return_json": True}),
                    ],
                ),
                PluginSource(
                    module_name="surplus_search.v8.surplus",
                    class_name="BlockData",
                    kwargs_list=[
                        PluginKwargs(kwargs={"return_json": True}),
                    ],
                ),
            ],
        ),
    )


def run(day: dt.date, yt_job_options: YtJobOptions, table_bounds: TableBounds, dest_table: str) -> None:
    squeezer = UserSessionsSqueezer(experiments=create_experiments_for_squeeze(), day=day)

    yt_spec = yt_io.get_yt_operation_spec(yt_pool=yt_job_options.pool,
                                          max_failed_job_count=1,
                                          data_size_per_job=yt_job_options.data_size_per_job * yt.common.MB,
                                          operation_executor_types=YtOperationTypeEnum.REDUCE,
                                          memory_limit=yt_job_options.memory_limit * yt.common.MB,
                                          enable_input_table_index=True,
                                          check_input_fully_consumed=True,
                                          max_row_weight=128 * yt.common.MB,
                                          title="Calculate surplus",
                                          use_porto_layer=False,
                                          acl=mstand_uyt.get_online_yt_operation_acl())

    reducer = SurplusCalculator(
        squeezer=squeezer,
        metric_container=create_metric_container(),
    )

    if table_bounds.lower_reducer_key and table_bounds.upper_reducer_key:
        bounds = f"[{table_bounds.lower_reducer_key}:{table_bounds.upper_reducer_key}]"
    elif table_bounds.lower_reducer_key:
        bounds = f"[{table_bounds.lower_reducer_key}:]"
    elif table_bounds.upper_reducer_key:
        bounds = f"[:{table_bounds.upper_reducer_key}]"
    else:
        bounds = ""

    logging.info("Run surplus calculation for day %s into table %s", day, dest_table)

    cluster = clusters.yt.Hahn()
    job = cluster \
        .job("Calculate surplus") \
        .env(
            yt_spec_defaults=yt_spec,
            merge_strategy=dict(final_tables="never"),
        )

    table_path = mstand_tables.UserSessionsTable(
        day=day, user_sessions="//user_sessions/pub", source=ServiceSourceEnum.SEARCH
    ).path

    if table_path is None:
        raise Exception("Failed getting table path.")

    sources = [
        job.table(
            table_path + bounds
        ),
    ]

    job.concat(*sources) \
        .libra(
            reducer,
            blockstat_dict_file=nfi.StatboxDict('blockstat.dict', use_latest=True),
            memory_limit=yt_job_options.memory_limit,
        ) \
        .put(dest_table)

    job.run()

    logging.info("Surplus calculation has finished successful")
