import json
import logging
import os
import random
import string
import time
from datetime import date, datetime
from typing import Optional

import settings
from common import async_logging
from agent.config.models import Prober, ProberConfig, ClusterMetadata
from common.monitoring.solomon import SolomonClient, SolomonMetric, MetricKind
from agent.status import ProberExecutionStatus
from agent.timer import Timer
from common.s3.logs.uploader import AsyncMrProberLogsS3Uploader
from database.models import UploadProberLogPolicy

# LIMIT_LOGGING_SIZE_DATA allows you to set a limit on the amount of data
# printed to the log for stdout and stderr of each prober run
# NOTE:
#  - the output is truncated in the middle
#  - the limit is applied separately on stdout and separately on stderr
LIMIT_LOGGING_SIZE_DATA = 512  # bytes


class ProberTelemetrySender:
    def __init__(
        self, solomon_client: SolomonClient,
        prober: Prober, config: ProberConfig, cluster: ClusterMetadata,
        limit_logging_size_data: int = LIMIT_LOGGING_SIZE_DATA,
    ):
        self._solomon_client = solomon_client
        self._prober = prober
        self._config = config
        self._cluster = cluster
        self._prober_logger = self._get_or_create_prober_logger()
        self.limit_logging_size_data = limit_logging_size_data

    def update_prober_and_config(self, prober: Prober, config: ProberConfig):
        self._prober = prober
        self._config = config

    def send_telemetry_about_prober_start(self):
        try:
            self._write_logs_about_start()
        except Exception as e:
            logging.error("Could not write logs about start: %s", e, exc_info=e)

    # https://black.readthedocs.io/en/stable/the_black_code_style/current_style.html#how-black-wraps-lines
    def send_telemetry_about_prober_execution(
        self, start_timestamp: int, duration_seconds: float, execution_status: ProberExecutionStatus,
        exit_code: Optional[int] = None, stdout: bytes = b"", stderr: bytes = b""
    ):
        if settings.SEND_METRICS_TO_SOLOMON:
            try:
                self._send_metrics_about_execution(
                    start_timestamp, duration_seconds,
                    execution_status == ProberExecutionStatus.SUCCESS,
                    stdout,
                    stderr,
                )
            except Exception as e:
                logging.error("Could not send metrics about execution to solomon-agent: %s", e, exc_info=e)

        try:
            self._write_logs_about_execution(
                start_timestamp,
                execution_status,
                exit_code,
                self.trim_data_in_the_middle(stdout, size=LIMIT_LOGGING_SIZE_DATA),
                self.trim_data_in_the_middle(stderr, size=LIMIT_LOGGING_SIZE_DATA)
            )
        except Exception as e:
            logging.error("Could not write logs about execution: %s", e, exc_info=e)

        try:
            self._upload_prober_log_into_s3(start_timestamp, execution_status, stdout, stderr)
        except Exception as e:
            logging.error("Could not upload logs about execution into S3: %s", e, exc_info=e)

    def _get_prober_log_filename(self) -> str:
        return f"{settings.MR_PROBER_PROBER_LOGS_PATH}/{self._prober.slug}.log"

    def _get_or_create_prober_logger(self) -> logging.Logger:
        _prober_logger = logging.getLogger(f"prober_{self._prober.slug}")

        if not _prober_logger.handlers:
            _prober_logger.setLevel(logging.INFO)
            _prober_logger.propagate = False

            os.makedirs(settings.MR_PROBER_PROBER_LOGS_PATH, exist_ok=True)
            fh = async_logging.AsyncWatchedFileHandler(self._get_prober_log_filename())
            fmt = logging.Formatter(fmt=settings.PROBER_LOG_FORMAT, datefmt=settings.LOG_DATE_FORMAT)
            fh.setFormatter(fmt)
            _prober_logger.addHandler(fh)

        return _prober_logger

    def _send_metrics_about_execution(
        self, timestamp: int, duration_seconds: float, is_success: bool, stdout: bytes, stderr: bytes
    ):
        sending_timer = Timer()

        solomon_labels = {
            "prober_slug": self._prober.slug,
            "cluster_slug": self._cluster.slug,
        }
        if self._config.additional_metric_labels:
            solomon_labels.update(self._config.additional_metric_labels)
        solomon_labels.update(settings.AGENT_ADDITIONAL_METRIC_LABELS)

        self._solomon_client.send_metrics(
            [
                SolomonMetric(
                    timestamp, 1 if is_success else 0, metric="success",
                    **solomon_labels,
                ),
                SolomonMetric(
                    timestamp, 0 if is_success else 1, metric="fail",
                    **solomon_labels,
                ),
                SolomonMetric(
                    timestamp, int(duration_seconds * 1000), metric="duration_milliseconds",
                    **solomon_labels,
                ),
                SolomonMetric(
                    timestamp, len(stdout), metric="stdout_size",
                    **solomon_labels,
                ),
                SolomonMetric(
                    timestamp, len(stderr), metric="stderr_size",
                    **solomon_labels,
                ),
            ]
        )

        logging.info(
            "Sent metrics about this execution to solomon-agent, "
            f"{sending_timer.get_total_seconds():.3f} seconds elapsed"
        )

    def _upload_prober_log_into_s3(
        self, timestamp: int, execution_status: ProberExecutionStatus, stdout: bytes, stderr: bytes
    ):
        need_send_logs = False
        if self._config.s3_logs_policy == UploadProberLogPolicy.ALL:
            need_send_logs = True
        if self._config.s3_logs_policy == UploadProberLogPolicy.FAIL and \
                execution_status != ProberExecutionStatus.SUCCESS:
            need_send_logs = True

        if need_send_logs:
            uploader = AsyncMrProberLogsS3Uploader(
                settings.S3_ENDPOINT,
                settings.S3_ACCESS_KEY_ID,
                settings.S3_SECRET_ACCESS_KEY,
                settings.MR_PROBER_LOGS_S3_BUCKET,
                settings.S3_PREFIX,
            )

            rnd = ''.join(random.choices(string.ascii_lowercase + string.digits, k=8))
            prefix = f"{date.fromtimestamp(timestamp)}/{datetime.fromtimestamp(timestamp)} " \
                     f"[{rnd}] {execution_status.name}"

            uploader.upload_prober_log(self._prober, self._cluster, settings.HOSTNAME, f"{prefix}.stdout.txt", stdout)
            uploader.upload_prober_log(self._prober, self._cluster, settings.HOSTNAME, f"{prefix}.stderr.txt", stderr)
        else:
            logging.debug(f"Uploading logs policy is {self._config.s3_logs_policy}, don't sent logs into S3")

    def _write_logs_about_start(self):
        self._prober_logger.info(f"Starting prober {self._prober.name!r} ({str(self._prober.runner)!r}) "
                                 f"on {self._prober.files_location.as_posix()}")
        self._prober_logger.info(f"Variable values: {self._config.variables!r}")

    def _write_logs_about_execution(
        self, timestamp: int, execution_status: ProberExecutionStatus,
        exit_code: Optional[int], stdout: bytes, stderr: bytes,
    ):
        try:
            stdout = stdout.decode("utf-8").rstrip("\n")
        except UnicodeDecodeError:
            pass
        try:
            stderr = stderr.decode("utf-8").rstrip("\n")
        except UnicodeDecodeError:
            pass

        if execution_status == ProberExecutionStatus.FAILED_TO_START:
            exit_message = "failed to start (see agent logs for details)"
        elif execution_status == ProberExecutionStatus.TIMEOUT:
            exit_message = "timed out"
        else:
            exit_message = f"exited with code {exit_code}"

        # CLOUD-100349. Print logs about failed runs to serial console (/dev/ttyS*) also if it's enabled via
        # environment variable $AGENT_TTY_DEVICE.
        if settings.AGENT_TTY_DEVICE and execution_status != ProberExecutionStatus.SUCCESS:
            try:
                with open(settings.AGENT_TTY_DEVICE, "w") as tty:
                    log_object = {"timestamp": timestamp, "prober_slug": self._prober.slug, "exit_code": exit_code,
                                  "additional_metric_labels": self._config.additional_metric_labels,
                                  "status": execution_status.name}
                    json.dump(log_object, tty)
                    tty.write("\n")
            except Exception as e:
                logging.warning(f"Can't print log into tty device {settings.AGENT_TTY_DEVICE}: {e}")
                logging.info(f"Configure tty device {settings.AGENT_TTY_DEVICE} or specify another via $AGENT_TTY_DEVICE")

        logging.info(f"Prober {self._prober.name!r} {exit_message}.")

        self._prober_logger.info(
            f"Prober {self._prober.name!r} {exit_message}.\n"
            f"-------------------------- [ STDOUT ] --------------------------\n"
            f"{stdout}\n"
            f"-------------------------- [ STDERR ] --------------------------\n"
            f"{stderr}\n"
            f"-------------------------- [  END   ] --------------------------\n"
        )
        logging.info(
            f"Saving stdout ({len(stdout)} bytes) and stderr ({len(stderr)} bytes) "
            f"to disk: {self._get_prober_log_filename()!r}"
        )

    @staticmethod
    def trim_data_in_the_middle(data: bytes, size: int) -> bytes:
        """
        This function tries to cut out the middle of the data, so that the size of the result is no larger than size.
        The cut data is replaced with the number of bytes cut.
        NOTE: Cutting works for size larger than the infix size, otherwise it will return the original data.
        """
        if size < 0:
            raise ValueError("data trim size '%s' must be non negative" % size)

        data_size = len(data)
        infix_pattern = b"[%d bytes truncated]"
        infix_size = len(infix_pattern) + len(str(size)) - 2
        if data_size <= size or size < infix_size:
            return data

        data_size_in_result = size - infix_size
        first_part_size = data_size_in_result // 2
        last_part_size = data_size_in_result - first_part_size
        truncated_size = data_size - data_size_in_result
        return data[:first_part_size] + infix_pattern % truncated_size + (
            data[-last_part_size:] if last_part_size else b"")


class AgentTelemetrySender:
    def __init__(self, solomon_client: SolomonClient, cluster_slug: str):
        self._solomon_client = solomon_client
        self._agent_start_time: Timer = Timer()
        self._cluster_slug = cluster_slug

    def send_agent_telemetry(self, probers_count: int):
        timestamp = int(time.time())
        try:
            self._solomon_client.send_metrics(
                [
                    SolomonMetric(
                        timestamp, 1, metric="keep_alive", kind=MetricKind.GAUGE, cluster_slug=self._cluster_slug,
                        **settings.AGENT_ADDITIONAL_METRIC_LABELS
                    ),
                    SolomonMetric(
                        timestamp, probers_count, metric="probers_count", kind=MetricKind.GAUGE,
                        cluster_slug=self._cluster_slug, **settings.AGENT_ADDITIONAL_METRIC_LABELS
                    ),
                    SolomonMetric(
                        timestamp, int(self._agent_start_time.get_total_seconds()), metric="uptime",
                        kind=MetricKind.COUNTER, cluster_slug=self._cluster_slug,
                        **settings.AGENT_ADDITIONAL_METRIC_LABELS
                    ),
                ]
            )
        except Exception as e:
            logging.error("Could not send agent metrics to solomon-agent: %s", e, exc_info=e)
