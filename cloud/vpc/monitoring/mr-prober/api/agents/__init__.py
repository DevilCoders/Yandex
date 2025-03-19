import logging
from itertools import chain
from typing import List

from sqlalchemy.orm import Session, joinedload

import database.models
import settings
from agent.config.models import Prober, Cluster, ProberConfig, ProberFile, BashProberRunner
from agent.config.s3.uploader import AgentConfigS3Uploader, AgentConfigS3DiffBuilder

__all__ = [
    "update_prober_in_s3", "update_cluster_in_s3", "update_clusters_in_s3",
    "update_all_clusters_in_s3", "update_all_probers_in_s3",
    "update_prober_file_in_s3", "uploader", "get_diff_builder",
    "get_prober_configs_with_null_cluster_id",
]

log = logging.getLogger(__name__)


def uploader():
    return AgentConfigS3Uploader(
        settings.S3_ENDPOINT,
        settings.S3_ACCESS_KEY_ID,
        settings.S3_SECRET_ACCESS_KEY,
        settings.AGENT_CONFIGURATIONS_S3_BUCKET,
        settings.S3_PREFIX,
        settings.S3_CONNECT_TIMEOUT,
        settings.S3_RETRY_ATTEMPTS,
    )


def get_diff_builder():
    return AgentConfigS3DiffBuilder(
        settings.S3_ENDPOINT,
        settings.S3_ACCESS_KEY_ID,
        settings.S3_SECRET_ACCESS_KEY,
        settings.AGENT_CONFIGURATIONS_S3_BUCKET,
        settings.S3_PREFIX,
        settings.S3_CONNECT_TIMEOUT,
        settings.S3_RETRY_ATTEMPTS,
    )


def update_prober_in_s3(uploader: AgentConfigS3Uploader, prober: database.models.Prober):
    log.info(f"Uploading prober config into S3 for agents configuration: {prober}")
    if isinstance(prober.runner, database.models.BashProberRunner):
        runner = BashProberRunner(command=prober.runner.command)
    else:
        raise ValueError(f"Unknown prober runner type: {type(prober.runner)}")

    uploader.upload_prober_config(
        Prober(
            id=prober.id,
            name=prober.name,
            slug=prober.slug,
            runner=runner,
            files=[ProberFile(
                id=file.id,
                is_executable=file.is_executable,
                relative_file_path=file.relative_file_path,
                md5_hexdigest=file.md5_hexdigest,
            ) for file in prober.files],
        )
    )


def update_prober_file_in_s3(uploader: AgentConfigS3Uploader, prober_file: database.models.ProberFile):
    log.info(f"Uploading prober file into S3 for agents configuration: {prober_file}")
    uploader.upload_prober_file(prober_file.prober_id, prober_file.id, prober_file.content)


def _update_cluster_in_s3(
    uploader: AgentConfigS3Uploader, cluster: database.models.Cluster,
    prober_configs_with_null_cluster_id: List[database.models.ProberConfig]
):
    log.info(f"Uploading cluster config into S3 for agents configuration: {cluster}")

    # Ignore cluster-specific configs for other clusters
    relevant_configs = list(chain(cluster.configs, prober_configs_with_null_cluster_id))

    # Prioritization of configs:
    # 1. Global configs (without cluster_id and hosts_re) have lowest priority
    # 2. Configs without cluster_id, but with hosts_re
    # 3. Configs with cluster_id specified, but without hosts_re
    # 4. Configs with both cluster_id and hosts_re should be latest
    relevant_configs.sort(key=lambda config: (config.cluster_id is not None, config.hosts_re is not None))

    log.info(f"Found {len(relevant_configs)} relevant prober configs: {relevant_configs!r}")

    agent_cluster = Cluster(
        id=cluster.id,
        name=cluster.name,
        slug=cluster.slug,
        variables={v.name: v.value for v in cluster.variables} if cluster.variables else {},
        prober_configs=[ProberConfig(
            prober_id=config.prober_id,
            hosts_re=config.hosts_re,
            is_prober_enabled=config.is_prober_enabled,
            interval_seconds=config.interval_seconds,
            timeout_seconds=config.timeout_seconds,
            s3_logs_policy=config.s3_logs_policy,
            default_routing_interface=config.default_routing_interface,
            dns_resolving_interface=config.dns_resolving_interface,
            matrix_variables={v.name: v.values for v in config.matrix_variables} if config.matrix_variables else {},
            variables={v.name: v.value for v in config.variables} if config.variables else {},
        ) for config in relevant_configs],
    )
    uploader.upload_cluster_config(agent_cluster)


def update_cluster_in_s3(db: Session, uploader: AgentConfigS3Uploader, cluster: database.models.Cluster):
    _update_cluster_in_s3(uploader, cluster, get_prober_configs_with_null_cluster_id(db))


def update_clusters_in_s3(
    uploader: AgentConfigS3Uploader, clusters: List[database.models.Cluster],
    prober_configs_with_null_cluster_id: List[database.models.ProberConfig]
):
    log.info(f"Uploading {len(clusters)} cluster configs into S3 for agents configuration")
    for cluster in clusters:
        _update_cluster_in_s3(uploader, cluster, prober_configs_with_null_cluster_id)


def update_all_clusters_in_s3(db: Session, uploader: AgentConfigS3Uploader):
    """
    Frequently one ProberConfig's update may have an impact on all agents in all clusters,
    so we should update all clusters in S3.
    """
    clusters = db.query(database.models.Cluster).options(joinedload(database.models.Cluster.configs)).all()
    update_clusters_in_s3(uploader, clusters, get_prober_configs_with_null_cluster_id(db))


def update_all_probers_in_s3(db: Session, uploader: AgentConfigS3Uploader):
    log.info("Uploading ALL prober config into S3 for agents configuration")
    probers = db.query(database.models.Prober)
    for prober in probers:
        update_prober_in_s3(uploader, prober)
        for file in prober.files:
            if file.content is not None:
                update_prober_file_in_s3(uploader, file)


def get_prober_configs_with_null_cluster_id(db: Session) -> List[database.models.ProberConfig]:
    return db.query(database.models.ProberConfig).filter(database.models.ProberConfig.cluster_id == None).all()
