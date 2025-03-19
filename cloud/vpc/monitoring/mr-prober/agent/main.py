#!/usr/bin/env python3
import logging
import logging.config
import os
import signal
import sys
import time
from typing import Dict, Tuple

import botocore.exceptions
import requests.exceptions

import settings
from agent.config import AbstractAgentConfigLoader, S3AgentConfigLoader
from agent.process import ProberProcess
from agent.telemetry import AgentTelemetrySender
from common.metadata import CloudMetadataClient
from common.monitoring.solomon import SolomonClient, AsyncSolomonClient
from common.util import start_daemon_thread

PROBER_PROCESSES_POLL_INTERVAL = 0.01  # seconds
MAX_PROBER_START_DELAY = 60  # seconds
CHECK_NEW_VERSION_INTERVAL = 60  # seconds


def update_probers_map_from_config(
    probers_map: Dict[Tuple, ProberProcess],
    config_loader: AbstractAgentConfigLoader,
    solomon_client: SolomonClient,
):
    try:
        config = config_loader.load()
    except Exception as e:
        logging.error("Can't refresh agent config from S3", exc_info=e)
        return

    for prober_index, prober_with_config in enumerate(config.probers):
        unique_key = prober_with_config.unique_key
        prober = prober_with_config.prober
        prober_config = prober_with_config.config
        if unique_key in probers_map:
            probers_map[unique_key].update_prober_if_changed(prober)
            probers_map[unique_key].update_config_if_changed(prober_config)
        else:
            delay_seconds = prober_index % MAX_PROBER_START_DELAY
            logging.info(f"New prober has been found, run it with delay {delay_seconds} seconds: {prober.name}")
            probers_map[unique_key] = ProberProcess(
                prober, prober_config, config.cluster, solomon_client, delay_seconds
            )

    # Delete probers which exists in probers_map but has been removed from config.probers
    for unique_key in set(probers_map.keys()) - {prober_with_config.unique_key for prober_with_config in
                                                 config.probers}:
        probers_map[unique_key].stop_forever()
        del probers_map[unique_key]


def load_config_with_retry(config_loader: AbstractAgentConfigLoader):
    for i in range(settings.AGENT_GET_CLUSTER_CONFIG_RETRY_ATTEMPTS):
        try:
            return config_loader.load()
        except botocore.exceptions.ClientError as e:
            logging.error(
                "Can't get agent config from S3. Attempt %d of %d.",
                i + 1,
                settings.AGENT_GET_CLUSTER_CONFIG_RETRY_ATTEMPTS,
                exc_info=e
            )
            time.sleep(settings.AGENT_GET_CLUSTER_CONFIG_RETRY_DELAY)
        except Exception as e:
            logging.error("Can't refresh agent config from S3", exc_info=e)
            break


def config_updating_loop(
    probers_map: Dict[Tuple, ProberProcess],
    config_loader: AbstractAgentConfigLoader,
    solomon_client: SolomonClient,
):
    """
    Updates config from API (via S3) every UPDATE_AGENT_CONFIG_FROM_S3_INTERVAL_SECONDS seconds
    """
    while True:
        time.sleep(settings.UPDATE_AGENT_CONFIG_FROM_S3_INTERVAL_SECONDS)

        update_probers_map_from_config(probers_map, config_loader, solomon_client)


def send_agent_metrics_loop(
    probers_map: Dict[Tuple, ProberProcess],
    solomon_client: SolomonClient,
    cluster_slug: str,
):
    """
    Sends agent's telemetry (probers count, keep alive and other metrics) every
    AGENT_TELEMETRY_SENDING_INTERVAL seconds
    """
    agent_telemetry_sender = AgentTelemetrySender(solomon_client, cluster_slug)
    while True:
        agent_telemetry_sender.send_agent_telemetry(len(probers_map))
        time.sleep(settings.AGENT_TELEMETRY_SENDING_INTERVAL)


def agent_version_updater_loop():
    """
    Assumes that $AGENT_DOCKER_IMAGE contains Docker Image ID (i.e. cr.yandex/crpni6s1s1aujltb5vv7/agent:1.10)
    of running container.
    Assumes also that metadata has `mr-prober-agent-docker-image` attribute.

    If so, agent compares these two strings. If they're different, it assumes that
    we should stop the agent, and external system (systemd) will start a fresh
    container with a new version of the agent.
    """
    if settings.AGENT_DOCKER_IMAGE is None:
        return

    metadata_client = CloudMetadataClient()
    attribute_name = "attributes/mr-prober-agent-docker-image"
    while True:
        try:
            agent_docker_image_from_metadata = metadata_client.get_metadata_value(attribute_name)
        except Exception as e:
            if not isinstance(e, requests.exceptions.HTTPError) or e.response.status_code != requests.codes.not_found:
                logging.warning(
                    f"Can't fetch agent docker image url from metadata: {e}. Will try again later", exc_info=e
                )
        else:
            if agent_docker_image_from_metadata != settings.AGENT_DOCKER_IMAGE:
                logging.info(f"Metadata attribute {attribute_name} has value {agent_docker_image_from_metadata}, "
                             f"Actual version in $AGENT_DOCKER_IMAGE is {settings.AGENT_DOCKER_IMAGE}, need restart")
                # See https://stackoverflow.com/questions/905189/why-does-sys-exit-not-exit-when-called-inside-a-thread-in-python
                # about how to stop program from a thread.
                try:
                    try:
                        os.kill(os.getpid(), signal.SIGINT)
                    finally:
                        # noinspection PyUnresolvedReferences,PyProtectedMember
                        os._exit(0)
                except Exception as e:
                    logging.error(f"Can not stop the service: {e}", exc_info=e)

        time.sleep(CHECK_NEW_VERSION_INTERVAL)


def probers_loop(probers_map: Dict[Tuple, ProberProcess]):
    while True:
        # We have to do a copy of dict.values(), because it's content can be modified from another thread.
        # Dict is a thread-safe container, but only for single operations such as adding or removing one key.
        for prober in list(probers_map.values()):
            # Check status of each prober and start it if it's needed
            prober.check_status()

            if prober.need_start():
                prober.start()

        time.sleep(PROBER_PROCESSES_POLL_INTERVAL)


def main(config_loader: AbstractAgentConfigLoader) -> int:
    probers_map: Dict[Tuple, ProberProcess] = {}
    solomon_client = AsyncSolomonClient()
    try:
        agent_config = load_config_with_retry(config_loader)
        if agent_config is None:
            logging.error("Can't get agent config from S3. All attempts are exceeded.")
            return 1

        # Load config for the first time
        update_probers_map_from_config(probers_map, config_loader, solomon_client)

        start_daemon_thread("ConfigUpdater", lambda: config_updating_loop(probers_map, config_loader, solomon_client))
        start_daemon_thread("AgentVersionUpdater", lambda: agent_version_updater_loop())
        if settings.SEND_METRICS_TO_SOLOMON:
            start_daemon_thread(
                "AgentTelemetry",
                lambda: send_agent_metrics_loop(probers_map, solomon_client, agent_config.cluster.slug)
            )

        probers_loop(probers_map)
    except KeyboardInterrupt:
        logging.info("Exiting...")


if __name__ == "__main__":
    logging.config.dictConfig(settings.LOGGING_CONFIG)

    if settings.CLUSTER_ID is None:
        logging.error("Can't determine cluster id. Specify CLUSTER_ID environment variable.")
        sys.exit(1)

    logging.info("Starting Mr. Prober agent for cluster id %d, hostname %s", settings.CLUSTER_ID, settings.HOSTNAME)

    s3_config_loader = S3AgentConfigLoader()
    sys.exit(main(s3_config_loader))
