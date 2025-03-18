import argparse

from ab_calculations.stat_fetcher import stat_fetcher
from quality.ab_testing.scripts.nirvana.get_calculations.lib import schemes
from quality.ab_testing.scripts.nirvana.get_calculations.lib.calculation import run_calculations
from quality.ab_testing.scripts.nirvana.tools.ab_adminka.adminka import AdminkaClient

from typing import List
from yaqtypes import JsonDict

from yaqutils import YaqEnum

import mstand_utils.args_helpers as mstand_uargs
from abt import result_parser
from experiment_pool import Observation


class BackendKeyEnum(YaqEnum):
    COFE = "cofe"
    SPU = "spu"
    STAT_FETCHER = "stat_fetcher"


CALC_TYPES_COFE = {"cofe"}
CALC_TYPES_SPU = {"spu-v2"}
CALC_TYPES_STAT_FETCHER = {"base", "zen", "bs"}

ALL_TYPES = sorted(CALC_TYPES_COFE | CALC_TYPES_SPU | CALC_TYPES_STAT_FETCHER)

BACKEND_TYPES = dict(
    **dict.fromkeys(CALC_TYPES_COFE, BackendKeyEnum.COFE),
    **dict.fromkeys(CALC_TYPES_SPU, BackendKeyEnum.SPU),
    **dict.fromkeys(CALC_TYPES_STAT_FETCHER, BackendKeyEnum.STAT_FETCHER),
)


def add_abt_fetch_args(parser):
    """
    :type parser: argparse.ArgumentParser
    """
    parser.add_argument(
        "--metric",
        nargs="*",
        help="list of metric names",
        default=[],
    )
    parser.add_argument(
        "--metric-groups",
        nargs="*",
        help="list of metric group names (only for base metrics)",
        default=[],
    )
    parser.add_argument(
        "--metric-type",
        default="base",
        help="metric type",
        choices=ALL_TYPES,
    )
    parser.add_argument(
        "--stat-fetcher-groups",
        nargs="*",
        help="list of stat-fetcher groups (possible values: {})".format(", ".join(schemes.ABT_GROUPS)),
        default=["clean"],
    )
    parser.add_argument(
        "--stat-fetcher-granularity",
        choices=stat_fetcher.STAT_FETCHER_GRANULARITIES,
        help="stat-fetcher granularity params",
        default="24",
    )
    parser.add_argument(
        "--cofe-project",
        default="collections",
        help="cofe project (see in ulr: ab.yandex-team.ru/observation/<id>/launch/<project>)",
    )
    parser.add_argument(
        "--yuid-only",
        action="store_true",
        help="pass yuid-only=yes to stat_long_fetcher",
    )
    parser.add_argument(
        "--detailed",
        action="store_true",
        help="detailed information about metrics (ttest, mwtest etc.) - base metrics only",
    )
    mstand_uargs.add_list_of_services(
        parser,
        default="www.yandex",
        help_message="use these services for f-metrics (default=www.yandex)",
    )
    parser.add_argument(
        "--custom-test",
        choices=["ttest", "mwtest", "waldtest"],
        help="get custom test from abt (ttest|mwtest|waldtest)",
    )
    parser.add_argument(
        "--spuv2-path",
        help="mr table path for SPUv2 metrics",
    )
    parser.add_argument(
        "--metric-picker-key",
        help="add metric_picker_key for ab metrics (USEREXP-8330)",
        default="preset_all",
    )
    parser.add_argument(
        "--force",
        action="store_true",
        help="need force recalc",
    )


class ParamsBuilder:
    def __init__(
            self,
            metric_names: List[str],
            metric_groups: List[str],
            metric_picker_key: str,
            metric_type: str,
            yuid_only: bool,
            services: List[str],
            detailed: bool,
            custom_test: str,
            spuv2_path: str,
            cofe_project: str,
            stat_fetcher_granularity: str,
            stat_fetcher_groups: List[str],
            force: bool,
    ) -> None:
        self.metric_names = metric_names
        self.metric_groups = metric_groups
        self.metric_picker_key = metric_picker_key
        self.metric_type = metric_type
        self.yuid_only = yuid_only
        self.services = services
        self.detailed = detailed
        self.custom_test = custom_test
        self.spuv2_path = spuv2_path
        self.cofe_project = cofe_project
        self.stat_fetcher_granularity = stat_fetcher_granularity
        self.stat_fetcher_groups = stat_fetcher_groups
        self.force = force

    @staticmethod
    def from_cli_args(cli_args: argparse.Namespace) -> "ParamsBuilder":
        return ParamsBuilder(
            metric_names=[name.strip() for name in cli_args.metric],
            metric_groups=[name.strip() for name in cli_args.metric_groups],
            metric_picker_key=cli_args.metric_picker_key,
            metric_type=cli_args.metric_type,
            yuid_only=cli_args.yuid_only,
            services=cli_args.services,
            detailed=cli_args.detailed,
            custom_test=cli_args.custom_test,
            spuv2_path=cli_args.spuv2_path,
            cofe_project=cli_args.cofe_project,
            stat_fetcher_granularity=cli_args.stat_fetcher_granularity,
            stat_fetcher_groups=cli_args.stat_fetcher_groups,
            force=cli_args.force,
        )

    def get_params(self, obs: Observation, token: str) -> JsonDict:
        data = dict(
            date_start=obs.dates.start.strftime("%Y-%m-%d"),
            date_finish=obs.dates.end.strftime("%Y-%m-%d"),
        )

        backend_type = BACKEND_TYPES[self.metric_type]
        if backend_type == BackendKeyEnum.COFE:
            data["calc_type"] = self.cofe_project
        elif backend_type == BackendKeyEnum.SPU:
            data["calc_type"] = "spu"

            if self.spuv2_path:
                data["table"] = self.spuv2_path
            if self.services:
                data["services"] = self.services
        elif backend_type == BackendKeyEnum.STAT_FETCHER:
            data["groups"] = self.stat_fetcher_groups
            data["granularity"] = self.stat_fetcher_granularity

            if self.metric_type == "base":
                data["calc_type"] = "abt"
            elif self.metric_type == "zen":
                data["calc_type"] = "zen"
            elif self.metric_type == "bs":
                data["calc_type"] = "bs"
            else:
                raise Exception("Was gotten unsupported backend type: {}".format(backend_type))
        else:
            raise Exception("Was gotten unsupported backend type: {}".format(backend_type))

        result_filters = dict(
            testids=[exp.testid for exp in obs.all_experiments()]
        )
        if self.metric_picker_key != "preset_all":
            result_filters["metric_picker_key"] = self.metric_picker_key
        else:
            adminka_client = AdminkaClient(token=token)
            result_filters["metric_picker_key"] = adminka_client.get_metric_picker(
                data["calc_type"],
                metric_keys=self.metric_names,
                group_keys=self.metric_groups
            )["key"]

        result = dict(
            obs_id=obs.id,
            data=data,
            result_filters=result_filters,
        )

        if self.detailed and self.metric_names:
            result["detailed_filters"] = dict(metric_keys=self.metric_names)

        return result


class ResultGetter(object):
    def __init__(
            self,
            metric_fetcher,
            metric_parser,
            params_builder: ParamsBuilder,
            token: str,
            coloring: str,
    ) -> None:
        self.metric_fetcher = metric_fetcher
        self.metric_parser = metric_parser
        self.params_builder = params_builder
        self.token = token
        self.coloring = coloring

    @staticmethod
    def from_cli_args(cli_args: argparse.Namespace) -> "ResultGetter":
        params_builder = ParamsBuilder.from_cli_args(cli_args)
        metric_fetcher = None
        token = mstand_uargs.get_token(cli_args.ab_token_file)

        if cli_args.metric_type in CALC_TYPES_SPU:
            metric_parser = result_parser.SpuResultParser()
        else:
            metric_parser = result_parser.ResultParser()

        return ResultGetter(
            metric_fetcher=metric_fetcher,
            metric_parser=metric_parser,
            params_builder=params_builder,
            token=token,
            coloring=cli_args.set_coloring,
        )

    def get_metric_result(self, observation: Observation) -> Observation:
        calc_params = [self.params_builder.get_params(observation, self.token)]

        result = list(run_calculations(
            calculations_info=calc_params,
            token=self.token,
            force=self.params_builder.force,
            # uncomment for testing
            # ems_host="https://beta.ems.yandex-team.ru",
            # adminka_host="https://ab.test.yandex-team.ru"
        ))

        testids_info = self.get_testids(result[0])
        assert testids_info == [exp.testid for exp in observation.all_experiments()]
        summary_info = self.get_summaries(result[0])
        daily_info = self.get_daily(result[0])
        self.metric_parser.parse(observation=observation, testids=testids_info,
                                 summary=summary_info, daily=daily_info, coloring=self.coloring)

        return observation

    @staticmethod
    def get_summaries(calc_result: JsonDict) -> JsonDict:
        fields = calc_result["data"]["info"]["filters"]["testid_fields"]
        summary = {}
        for metric_data in calc_result["data"]["data"]:
            metric_name = metric_data["key"]
            metrics = [
                dict(zip(fields, values))
                for values in metric_data["data"]
            ]
            summary[metric_name] = metrics
        return summary

    @staticmethod
    def get_daily(calc_result: JsonDict) -> JsonDict:
        daily = {}
        for metric_data in calc_result.get('detailed', []):
            metric_name = metric_data["info"]["filters"]["metric_key"]
            fields = metric_data["info"]["filters"]["testid_fields"]
            metrics = []
            for day_data in metric_data["data"]:
                metrics.append([
                    dict(zip(fields, values))
                    for values in day_data["data"]
                ])
            daily[metric_name] = metrics
        return daily

    @staticmethod
    def get_testids(calc_result: JsonDict) -> List[str]:
        return calc_result["data"]["info"]["filters"]["testids"]
