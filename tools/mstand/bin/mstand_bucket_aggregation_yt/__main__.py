#!/usr/bin/env python3
import argparse
import logging
import os.path
import shutil
import time

import yt.wrapper as yt

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.client_yt as client_yt
import postprocessing.scripts as pp_scripts
import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc
import yaqutils.time_helpers as utime

from dataclasses import dataclass

from typing import List

from yaqtypes import JsonDict

from experiment_pool import pool_helpers
from experiment_pool.experiment import Experiment
from experiment_pool.metric_result import MetricResult
from experiment_pool.metric_result import MetricValues
from experiment_pool.observation import Observation
from postprocessing.scripts.buckets import BucketModes


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="mstand buckets aggregation using yt")
    uargs.add_verbosity(parser)
    uargs.add_threads(parser, default=None)

    uargs.add_input(parser, help_message="path to input pool file")
    uargs.add_output(parser, help_message="write output pool to this file")

    mstand_uargs.add_save_to_dir(parser)
    mstand_uargs.add_save_to_tar(parser)
    mstand_uargs.add_yt(parser)

    uargs.add_boolean_argument(
        parser,
        name="skip_experiments_with_emtpy_buckets",
        help_message="skip experiments with emtpy buckets",
        default=False,
    )
    parser.add_argument("--num-buckets", type=int, default=100, help="number of buckets")
    parser.add_argument(
        "--aggregate-by",
        default=BucketModes.AVERAGE,
        help="aggregate by",
        choices=BucketModes.ALL,
    )

    return parser.parse_args()


@yt.aggregator
class BucketAggregationMapper:
    def __init__(self, column_name: str, postprocessor: pp_scripts.buckets.BucketPostprocessor):
        self.column_name = column_name
        self.postprocessor = postprocessor

    def __call__(self, rows):
        buckets = self.postprocessor.do_buckets_division(exp=(row[self.column_name] for row in rows))
        for i, bucket in enumerate(buckets):
            yield dict(sum=bucket.sum, length=bucket.length, bucket=i)


def update_metric_results(
        metric_result: MetricResult,
        dest_filename: str,
        values: List[float],
) -> None:
    new_sum_val = sum(values)
    new_count_val = len(values)
    old_data_type = metric_result.metric_values.data_type
    old_value_type = metric_result.metric_values.value_type
    metric_result.metric_values = MetricValues(
        count_val=new_count_val,
        sum_val=new_sum_val,
        average_val=new_sum_val / new_count_val if values else None,
        row_count=new_count_val,
        data_file=dest_filename,
        data_type=old_data_type,
        value_type=old_value_type,
    )


@dataclass
class BucketAggregationTask:
    observation: Observation
    experiment: Experiment
    metric_result: MetricResult
    postprocessor: pp_scripts.BucketPostprocessor
    yt_config: JsonDict
    dest_dir: str

    @property
    def dest_filename(self) -> str:
        key = "_".join([
            "bucket_aggregation",
            str(self.observation.id),
            self.experiment.testid,
            utime.format_date(self.observation.dates.start),
            utime.format_date(self.observation.dates.end),
            self.metric_result.result_table_field,
        ])
        return f"{key}.tsv"

    def __str__(self) -> str:
        return f"BucketAggregationTask -> {self.dest_filename}"


def make_bucket_aggregation(task: BucketAggregationTask) -> None:
    logging.info("Start bucket aggregation for %s", task)

    client = yt.YtClient(config=task.yt_config)
    with client.TempTable(prefix="bucket_aggregation_") as tmp_table:
        logging.info("Process table %s for task %s", task.metric_result.result_table_path, task)
        client.run_map(
            BucketAggregationMapper(
                column_name=task.metric_result.result_table_field,
                postprocessor=task.postprocessor,
            ),
            source_table=task.metric_result.result_table_path,
            destination_table=tmp_table,
        )

        logging.info("Download and aggregate results for %s", task)
        buckets = [pp_scripts.buckets.Bucket() for _ in range(task.postprocessor.num_buckets)]
        for row in client.read_table(tmp_table):
            bucket = buckets[row["bucket"]]
            bucket.add_value(row["sum"], row["length"])
        result = list(task.postprocessor.collect_bucket_results(buckets))

    dest_path = os.path.join(task.dest_dir, task.dest_filename)
    logging.info("Save results into %s", dest_path)
    with ufile.fopen_write(dest_path, use_unicode=False) as f:
        f.write("\n".join(str(v) for v in result))

    logging.info("Update metric results for task %s", task)
    update_metric_results(task.metric_result, task.dest_filename, result)


def main():
    cli_args = parse_args()
    umisc.configure_logger(cli_args.verbose, cli_args.quiet)

    if not cli_args.save_to_tar and not cli_args.save_to_dir:
        raise Exception("Output not specified. Please specify --save-to-dir or --save-to-tar parameter.")

    if cli_args.save_to_dir:
        dest_dir_is_tmp = False
        dest_dir = cli_args.save_to_dir
        ufile.make_dirs(dest_dir)
    else:
        dest_dir = ufile.create_temp_dir(prefix="bucket_aggregation_", subdir="mstand")
        dest_dir_is_tmp = True

    start_time = time.time()

    pool = pool_helpers.load_pool(cli_args.input_file)

    postprocessor = pp_scripts.BucketPostprocessor(
        num_buckets=cli_args.num_buckets,
        aggregate_by=cli_args.aggregate_by,
        flatten_array=True,
        skip_experiments_with_emtpy_buckets=cli_args.skip_experiments_with_emtpy_buckets,
    )

    yt_config = client_yt.create_yt_config_from_cli_args(cli_args)
    if cli_args.yt_pool:
        yt_config["pool"] = cli_args.yt_pool

    tasks = []
    for obs in pool.observations:
        for exp in obs.all_experiments():
            for mr in exp.metric_results:
                task = BucketAggregationTask(
                    observation=obs,
                    experiment=exp,
                    metric_result=mr,
                    postprocessor=postprocessor,
                    yt_config=yt_config,
                    dest_dir=dest_dir,
                )
                tasks.append(task)

    for _ in umisc.par_imap(make_bucket_aggregation, tasks, cli_args.threads, dummy=True):
        pass

    umisc.log_elapsed(start_time, "pure bucket aggregation time")

    pool_helpers.dump_pool(pool, os.path.join(dest_dir, "pool.json"))

    if cli_args.output_file:
        pool_helpers.dump_pool(pool, cli_args.output_file)

    if cli_args.save_to_tar:
        ufile.tar_directory(path_to_pack=dest_dir, tar_name=cli_args.save_to_tar, dir_content_only=True)

    if dest_dir_is_tmp:
        logging.info("removing tmp dest dir: %s", dest_dir)
        shutil.rmtree(dest_dir)


if __name__ == "__main__":
    main()
