import logging
from datetime import datetime
from typing import AnyStr, Optional, Dict

import pytz

import settings
from agent.timer import Timer
from common.monitoring.solomon import SolomonClient, SolomonMetric
from common.s3.logs.uploader import MrProberLogsS3Uploader
from database.models import ClusterDeployment, ClusterDeploymentStatus, Cluster


class CreatorTelemetrySender:
    def __init__(self, logger: Optional[logging.Logger] = None):
        start_datetime = datetime.now(pytz.utc)
        self._prefix = f"{start_datetime.date()}/{start_datetime.strftime('%Y-%m-%d %H:%M:%S %Z')}"
        self.start_timestamp = int(start_datetime.timestamp())
        self.logs = {}
        self._uploader = MrProberLogsS3Uploader(
            settings.S3_ENDPOINT,
            settings.S3_ACCESS_KEY_ID,
            settings.S3_SECRET_ACCESS_KEY,
            settings.MR_PROBER_LOGS_S3_BUCKET,
            settings.S3_PREFIX,
        )
        self.solomon_client = SolomonClient()
        self._logger = logger if logger else logging.getLogger(self.__class__.__name__)

    def collect_logs(self, cmd: str, stdout: AnyStr, stderr: AnyStr):
        self.logs[cmd] = (stdout, stderr)

    def upload_tf_plan_into_s3(self, deployment: ClusterDeployment, plan_human_readable: str, plan_tf: bytes):
        self._upload_tf_log_into_s3(deployment, "terraform plan.stdout.txt", plan_human_readable)

        extra_args = {'ContentType': 'application/octet-stream'}
        self._upload_tf_log_into_s3(deployment, "plan.tfplan", plan_tf, extra_args)

    def upload_tf_apply_log_into_s3(self, deployment: ClusterDeployment, out: AnyStr):
        self._upload_tf_log_into_s3(deployment, "apply.log", out)

    def upload_failed_tf_logs_into_s3(self, deployment: ClusterDeployment, stdout: AnyStr, stderr: AnyStr):
        if deployment.status == ClusterDeploymentStatus.INIT_FAILED:
            cmd = "terraform init"
        elif deployment.status == ClusterDeploymentStatus.PLAN_FAILED:
            cmd = "terraform plan"
        else:
            cmd = "terraform apply"
        # Uploading logs for failed step
        self._upload_tf_log_into_s3(deployment, f"{cmd} {deployment.status}.stdout.txt", stdout)
        self._upload_tf_log_into_s3(deployment, f"{cmd} {deployment.status}.stderr.txt", stderr)

        # Uploading logs for previous successful steps
        for cmd, logs in self.logs.items():
            out, err = logs
            self._upload_tf_log_into_s3(deployment, f"{cmd}.stdout.txt", out)
            self._upload_tf_log_into_s3(deployment, f"{cmd}.stderr.txt", err)

    def _upload_tf_log_into_s3(
        self, deployment: ClusterDeployment, filename: str, out: AnyStr, s3_extra_args: Optional[Dict[str, str]] = None
    ):
        if not out:
            return

        if isinstance(out, str):
            out = out.encode(errors="ignore")

        sending_timer = Timer()
        try:
            log_filename = self._uploader.upload_cluster_deployment_log(
                deployment.cluster, f"{self._prefix} [id={deployment.id}]/{filename}", out, s3_extra_args or {}
            )
        except Exception as e:
            self._logger.error(f"Can't upload cluster deployment log {filename} into Object Storage: {e}", exc_info=e)
        else:
            self._logger.info(
                f"Uploaded terraform {filename} into S3: {log_filename!r}, "
                f"{sending_timer.get_total_seconds():.3f} seconds elapsed"
            )

    def send_deployment_metrics(self, cluster: Cluster, deployment_status: ClusterDeploymentStatus):
        self.solomon_client.send_metrics([
            SolomonMetric(
                self.start_timestamp, 1,
                cluster_slug=cluster.slug, metric="deployment", status=deployment_status.value)
        ])
