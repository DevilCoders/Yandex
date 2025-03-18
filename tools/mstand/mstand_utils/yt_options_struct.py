import argparse
import os

from typing import Optional

import mstand_utils.args_helpers as mstand_uargs

from mstand_enums.mstand_general_enums import YsonFormats, YtDownloadingFormats


class YtJobOptions:
    def __init__(
            self,
            pool=None,
            data_size_per_job=300,
            max_data_size_per_job=200,
            memory_limit=None,
            tentative_enable=None,
            tentative_sample_count=None,
            tentative_duration_ratio=None,
    ):
        """
        :type pool: str | None
        :type memory_limit: int | None
        :type data_size_per_job: int
        :type max_data_size_per_job: int
        :type tentative_enable: bool | None
        :type tentative_sample_count: int | None
        :type tentative_duration_ratio: float | None
        """
        self.pool = pool
        self.data_size_per_job = data_size_per_job
        self.max_data_size_per_job = max_data_size_per_job
        self.memory_limit = memory_limit
        self.tentative_enable = tentative_enable
        self.tentative_sample_count = tentative_sample_count
        self.tentative_duration_ratio = tentative_duration_ratio

    @staticmethod
    def from_cli_args(cli_args):
        return YtJobOptions(
            pool=cli_args.yt_pool,
            data_size_per_job=getattr(cli_args, "yt_data_size_per_job", None),
            max_data_size_per_job=getattr(cli_args, "max_data_size_per_job", 200),
            memory_limit=cli_args.yt_memory_limit,
            tentative_enable=cli_args.yt_tentative_enable,
            tentative_sample_count=cli_args.yt_tentative_sample_count,
            tentative_duration_ratio=cli_args.yt_tentative_duration_ratio,
        )

    def get_tentative_job_spec(self):
        if not all([self.tentative_enable, self.tentative_sample_count, self.tentative_duration_ratio]):
            return {}

        return {
            "pool_trees": ["physical"],
            "tentative_pool_trees": ["cloud"],
            "tentative_tree_eligibility": {
                "sample_job_count": self.tentative_sample_count,
                "max_tentative_job_duration_ratio": self.tentative_duration_ratio,
            },
        }


class TableBounds:
    def __init__(
            self,
            lower_reducer_key: Optional[str] = None,
            upper_reducer_key: Optional[str] = None,
            start_mapper_index: Optional[str] = None,
            end_mapper_index: Optional[str] = None,
    ) -> None:
        self.lower_reducer_key = lower_reducer_key
        self.upper_reducer_key = upper_reducer_key
        self.start_mapper_index = start_mapper_index
        self.end_mapper_index = end_mapper_index

    @staticmethod
    def from_cli_args(cli_args: argparse.Namespace) -> "TableBounds":
        return TableBounds(
            lower_reducer_key=cli_args.lower_reducer_key or os.environ.get("DEBUG_LOWER_REDUCER_KEY"),
            upper_reducer_key=cli_args.upper_reducer_key or os.environ.get("DEBUG_UPPER_REDUCER_KEY"),
            start_mapper_index=cli_args.start_mapper_index,
            end_mapper_index=cli_args.end_mapper_index,
        )

    @staticmethod
    def add_cli_args(parser: argparse.ArgumentParser) -> None:
        mstand_uargs.add_lower_reducer_key(parser=parser)
        mstand_uargs.add_upper_reducer_key(parser=parser)
        mstand_uargs.add_start_mapper_index(parser=parser)
        mstand_uargs.add_end_mapper_index(parser=parser)

    def __contains__(self, item: str) -> bool:
        if self.lower_reducer_key and item < self.lower_reducer_key:
            return False
        if self.upper_reducer_key and item >= self.upper_reducer_key:
            return False
        return True


class TableFilterBounds:
    def __init__(
            self,
            lower_filter_key: str,
            upper_filter_key: str,
    ) -> None:
        self.lower_filter_key = lower_filter_key
        self.upper_filter_key = upper_filter_key

    @staticmethod
    def from_cli_args(cli_args: argparse.Namespace) -> "TableFilterBounds":
        return TableFilterBounds(
            lower_filter_key=cli_args.lower_filter_key,
            upper_filter_key=cli_args.upper_filter_key,
        )

    @staticmethod
    def add_cli_args(parser: argparse.ArgumentParser) -> None:
        mstand_uargs.add_lower_filter_key(parser=parser)
        mstand_uargs.add_upper_filter_key(parser=parser)


class TableSavingParams:
    def __init__(
        self,
        dumping_format: str = YtDownloadingFormats.YSON,
        save_to_dir: Optional[str] = None,
        save_to_yt_dir: Optional[str] = None,
        yson_format: str = YsonFormats.BINARY,
        encode_utf8: bool = False,
    ) -> None:
        self.dumping_format = dumping_format
        self.save_to_dir = save_to_dir
        self.save_to_yt_dir = save_to_yt_dir
        self.yson_format = yson_format
        self.encode_utf8 = encode_utf8

    @staticmethod
    def from_cli_args(cli_args: argparse.Namespace) -> "TableSavingParams":
        return TableSavingParams(
            dumping_format=cli_args.dumping_format,
            save_to_dir=cli_args.save_to_dir,
            save_to_yt_dir=cli_args.save_to_yt_dir,
            yson_format=cli_args.yson_format,
            encode_utf8=cli_args.encode_utf8,
        )

    @staticmethod
    def add_cli_args(parser: argparse.ArgumentParser) -> None:
        mstand_uargs.add_local_dumping_format(parser=parser)
        mstand_uargs.add_save_to_dir(parser=parser, help_message="local directory to save result tables to")
        mstand_uargs.add_save_to_yt_dir(parser=parser, help_message="yt directory to save result tables to")
        mstand_uargs.add_yson_format(parser=parser)
        mstand_uargs.add_encode_utf8_flag(parser=parser)


class ResultOptions:
    def __init__(
            self,
            copy_result_to=None,
            results_only=False,
            results_ttl=None,
            split_metric_results=False,
            split_values_and_schematize=False,
    ):
        """
        :type copy_result_to: str | None
        :type results_only: bool | None
        :type results_ttl: int | None
        :type split_metric_results: bool
        :type split_values_and_schematize: bool
        """
        self.copy_result_to = copy_result_to
        self.results_only = results_only
        self.results_ttl = results_ttl
        self.split_metric_results = split_metric_results
        self.split_values_and_schematize = split_values_and_schematize

    @staticmethod
    def from_cli_args(cli_args):
        return ResultOptions(
            copy_result_to=cli_args.copy_results_to_yt_dir,
            results_only=cli_args.yt_results_only,
            results_ttl=cli_args.yt_results_ttl,
            split_metric_results=cli_args.split_metric_results,
            split_values_and_schematize=cli_args.split_values_and_schematize,
        )
