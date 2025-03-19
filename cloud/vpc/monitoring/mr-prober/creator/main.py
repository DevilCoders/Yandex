#!/usr/bin/env python3
import logging
import logging.config
import pathlib
import time
from datetime import datetime
from typing import List, Dict, Optional

import pytz
import yaml
import sqlalchemy.exc
from sqlalchemy.orm import joinedload, Session

import database
import settings
from common.monitoring.solomon import SolomonClient, SolomonMetric, MetricKind
from common.util import start_daemon_thread
from creator import terraform
from creator.telemetry import CreatorTelemetrySender
from database.models import ClusterDeploymentStatus, Cluster, ClusterDeployment, ClusterDeployPolicyType, \
    ClusterRecipe, ClusterDeployPolicy
from python_terraform import TerraformCommandError

SLEEP_INTERVAL_SECONDS = 60


def get_clusters_list(db) -> List[Cluster]:
    return list(db.query(Cluster).all())


def get_cluster(db, cluster_id: int) -> Cluster:
    """
    Returns cluster with all related data required for TerraformEnvironment
    """
    return db.query(Cluster).filter(Cluster.id == cluster_id).options(
        joinedload(Cluster.recipe).options(joinedload(ClusterRecipe.files)),
        joinedload(Cluster.variables),
        joinedload(Cluster.deploy_policy),
    ).first()


def add_cluster_deployment(
    cluster: Cluster, start_time: datetime, status: ClusterDeploymentStatus, db: Session
) -> ClusterDeployment:
    record = ClusterDeployment(cluster=cluster, status=status, start=start_time)
    db.add(record)
    db.commit()
    return record


def finish_cluster_deployment(
    cluster: Cluster,
    record: ClusterDeployment,
    status: ClusterDeploymentStatus,
    db: Session
) -> ClusterDeployment:
    finish_time = datetime.now(pytz.utc)

    record.status = status
    record.finish = finish_time

    cluster.last_deploy_attempt_finish_time = finish_time
    db.commit()
    return record


def process_failed_cluster_deployment(
    db: Session, cluster: Cluster, deployment: ClusterDeployment, deployment_start_time: datetime,
    deployment_status: ClusterDeploymentStatus, error: TerraformCommandError, telemetry_sender: CreatorTelemetrySender
):
    if deployment is None:
        deployment = add_cluster_deployment(cluster, deployment_start_time, deployment_status, db)

    try:
        deployment = finish_cluster_deployment(
            cluster, deployment, deployment_status, db
        )
    except sqlalchemy.exc.SQLAlchemyError as e:
        logging.error(f"Can't save cluster deployment into database: {e}", exc_info=e)

    telemetry_sender.upload_failed_tf_logs_into_s3(deployment, error.out, error.err)
    telemetry_sender.send_deployment_metrics(cluster, deployment_status)


def get_plan_kwargs(deploy_policy: ClusterDeployPolicy) -> Dict:
    kwargs = {}
    if deploy_policy is not None:
        kwargs["parallelism"] = deploy_policy.parallelism
        kwargs["timeout"] = deploy_policy.plan_timeout
    return kwargs


def get_apply_kwargs(deploy_policy: ClusterDeployPolicy) -> Dict:
    kwargs = {}
    if deploy_policy is not None:
        kwargs["parallelism"] = deploy_policy.parallelism
        kwargs["timeout"] = deploy_policy.apply_timeout
    return kwargs


def process_cluster(cluster: Cluster, db: Session):
    logger = logging.getLogger(cluster.slug)

    logger.info(f"Processing cluster {cluster.name} (id={cluster.id})")
    deployment_start_time = datetime.now(pytz.utc)
    deployment: Optional[ClusterDeployment] = None
    deployment_status: ClusterDeploymentStatus = ClusterDeploymentStatus.INIT_FAILED
    telemetry_sender = CreatorTelemetrySender(logger)

    if not cluster.is_ready_for_deploy():
        return

    try:
        with terraform.TerraformEnvironment(cluster) as tf:
            try:
                # Try to terraform init
                try:
                    _, stdout, stderr = tf.init()
                    telemetry_sender.collect_logs("terraform init", stdout, stderr)
                except TerraformCommandError as e:
                    # This is temporary code: it's needed only once for migrating remote state
                    if "terraform-registry.storage.yandexcloud.net" in e.err:
                        tf.migrate_to_yc_terraform_mirror()
                        logger.info(
                            "Successfully migrated terraform state to Yandex Cloud's terraform mirror. "
                            "Let's try to re-init terraform"
                        )
                        _, stdout, stderr = tf.init()
                        telemetry_sender.collect_logs("terraform init", stdout, stderr)
                    else:
                        raise

                # Try to terraform plan
                deployment_status = ClusterDeploymentStatus.PLAN_FAILED
                plan_result = tf.plan(**get_plan_kwargs(cluster.deploy_policy))

                if not plan_result:
                    # None means empty diff and empty plan
                    telemetry_sender.send_deployment_metrics(cluster, ClusterDeploymentStatus.COMPLETED_WITH_EMPTY_PLAN)
                    return

                _, plan_stdout, plan_stderr, plan_content = plan_result
                telemetry_sender.collect_logs("terraform plan", plan_stdout, plan_stderr)

                # Try to terraform apply
                deployment_status = ClusterDeploymentStatus.RUNNING
                deployment = add_cluster_deployment(cluster, deployment_start_time, deployment_status, db)
                telemetry_sender.upload_tf_plan_into_s3(deployment, plan_stdout, plan_content)
                deployment_status = ClusterDeploymentStatus.APPLY_FAILED

                _, out, _ = tf.apply(plan_content, **get_apply_kwargs(cluster.deploy_policy))
                # We do not collect_logs here,
                # because in case of success, stdout has been uploaded into s3 below,
                # and in case of any error, stdout and stderr are uploaded into s3 in the except block
                deployment_status = ClusterDeploymentStatus.COMPLETED

                try:
                    deployment = finish_cluster_deployment(
                        cluster, deployment, deployment_status, db
                    )
                except sqlalchemy.exc.SQLAlchemyError as e:
                    logger.error(f"Can't save cluster deployment into database: {e}", exc_info=e)

                # unset ship in case of manual mode
                if cluster.deploy_policy is not None and cluster.deploy_policy.type == ClusterDeployPolicyType.MANUAL:
                    try:
                        cluster.deploy_policy.ship = False
                        db.commit()
                    except sqlalchemy.exc.SQLAlchemyError as e:
                        logger.error(f"Can't unset cluster's 'ship' flag for manual deployed cluster: {e}. "
                                     f"Most probably, it will be deployed one more time", exc_info=e)

                telemetry_sender.upload_tf_apply_log_into_s3(deployment, out)
                telemetry_sender.send_deployment_metrics(cluster, deployment_status)
            except TerraformCommandError as e:
                logger.error(f"Terraform command failed: {e}", exc_info=e)
                if e.out:
                    logger.warning("Stdout: %r", e.out)
                if e.err:
                    logger.warning("Stderr: %r", e.err)

                process_failed_cluster_deployment(
                    db, cluster, deployment, deployment_start_time, deployment_status, e, telemetry_sender
                )
            except Exception as e:
                logging.error(f"Something gone wrong: {e}", exc_info=e)
    except Exception as e:
        logging.error(
            f"Error on initializing terraform environment for cluster {cluster}, skipping this cluster", exc_info=e
        )


def process_all_clusters(db: Session):
    for cluster in get_clusters_list(db):
        # get cluster with all related data from DB
        cluster = get_cluster(db, cluster.id)
        process_cluster(cluster, db)


def prepare_ycp_config(ycp_config_path: pathlib.Path, mr_prober_sa_authorized_key: dict):
    # If $MR_PROBER_SA_AUTHORIZED_KEY is empty, just ignore and do nothing with ycp config
    if not mr_prober_sa_authorized_key:
        return

    logging.info(f"Going to insert SA's authorized key into ycp config: {ycp_config_path.as_posix()}")

    with ycp_config_path.open() as ycp_config_file:
        ycp_config = yaml.safe_load(ycp_config_file)

    if "users" not in ycp_config or "sa" not in ycp_config["users"]:
        logging.error(f"Could not find users:sa section in {ycp_config_path.as_posix()}")
        return

    ycp_config["users"]["sa"]["service-account-key"] = mr_prober_sa_authorized_key

    with ycp_config_path.open("w") as ycp_config_file:
        yaml.safe_dump(ycp_config, ycp_config_file, indent=4)

    logging.info(f"Successfully updated ycp config at {ycp_config_path.as_posix()}")


def send_creator_metrics_loop():
    """
    Sends Creator's keep-alive every CREATOR_TELEMETRY_SENDING_INTERVAL seconds
    """
    solomon_client = SolomonClient()
    while True:
        solomon_client.send_metrics([SolomonMetric(int(time.time()), 1, metric="keep_alive", kind=MetricKind.GAUGE)])
        time.sleep(settings.CREATOR_TELEMETRY_SENDING_INTERVAL)


def main():
    # While running inside the docker container we need to prepare ycp config first:
    # write SA's authorized key into "users" section
    prepare_ycp_config(pathlib.Path("/root/.config/ycp/config.yaml"), settings.MR_PROBER_SA_AUTHORIZED_KEY)

    if settings.SEND_METRICS_TO_SOLOMON:
        start_daemon_thread(
            "AgentTelemetry",
            lambda: send_creator_metrics_loop()
        )

    while True:
        db = database.session_maker()
        try:
            process_all_clusters(db)
        except Exception as e:
            logging.error(f"Error occurred while processing clusters: {e}", exc_info=e)
        finally:
            db.close()

        logging.info(f"Sleep for {SLEEP_INTERVAL_SECONDS} seconds before next iteration")
        time.sleep(SLEEP_INTERVAL_SECONDS)


if __name__ == "__main__":
    logging.config.dictConfig(settings.LOGGING_CONFIG)
    database.connect(settings.DATABASE_URL, settings.DATABASE_CONNECT_ARGS)
    main()
