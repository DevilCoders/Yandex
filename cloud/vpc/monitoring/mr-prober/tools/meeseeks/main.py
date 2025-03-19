#!/usr/bin/env python3
import enum
import itertools
import json
import logging
import pathlib
import time
from typing import List, Tuple, Dict

import click
import jwt
from rich import box
from rich.table import Table, Column

import api.client
import common.grpc
import settings
from common.monitoring.solomon import SolomonClient, SolomonMetric
from tools.common import main, cli, read_api_key, StopCliProcess, console, print_success_panel, print_warning_panel, \
    ExitCode
from yandex.cloud.priv.compute.v1.admin.node_pb2 import Node, NodeStatus
from yandex.cloud.priv.compute.v1.admin.node_service_pb2 import ListNodesRequest, ListNodesResponse
from yandex.cloud.priv.compute.v1.admin.node_service_pb2_grpc import NodeServiceStub
from yandex.cloud.priv.iam.v1.iam_token_service_pb2 import CreateIamTokenRequest, CreateIamTokenResponse
from yandex.cloud.priv.iam.v1.iam_token_service_pb2_grpc import IamTokenServiceStub


class MeeseeksUpdateStatus(enum.Enum):
    UPDATED = "UPDATED"
    EMPTY_DIFF = "EMPTY_DIFF"
    TOO_MUCH_TO_REMOVE = "TOO_MUCH_TO_REMOVE"
    INTERNAL_ERROR = "INTERNAL_ERROR"
    INVALID_USAGE = "INVALID_USAGE"


def report_update_status_metric(timestamp: int, status: MeeseeksUpdateStatus):
    if not settings.SEND_METRICS_TO_SOLOMON:
        return

    solomon_client = SolomonClient()
    solomon_client.send_metrics([
        SolomonMetric(timestamp, 1, subservice="meeseeks-updater", metric="update_event", status=status.value)
    ])


def report_meeseeks_count_metric(timestamp: int, count: int):
    if not settings.SEND_METRICS_TO_SOLOMON:
        return

    solomon_client = SolomonClient()
    solomon_client.send_metrics([
        SolomonMetric(timestamp, count, subservice="meeseeks-updater", metric="compute_nodes_count")
    ])


def _find_cluster_and_variable(cluster_slug: str, mr_prober_client: api.client.MrProberApiClient, variable_name: str):
    clusters = mr_prober_client.clusters.list()
    for cluster in clusters:
        if cluster.slug == cluster_slug:
            break
    else:
        cluster_slugs = [cluster.slug for cluster in clusters]
        raise StopCliProcess(
            f"Cluster with slug {cluster_slug!r} not found in Mr. Prober API. "
            f"Available clusters: {cluster_slugs!r}."
        )
    console.print(f"[bold]Found cluster[/bold] {cluster.id} with slug {cluster_slug!r}: {cluster.name!r}")
    for variable in cluster.variables:
        if variable.name == variable_name:
            break
    else:
        variable_names = [variable.name for variable in cluster.variables]
        raise StopCliProcess(
            f"Variable with name {variable_name!r} not found in Mr. Prober Cluster. "
            f"Available variables: {variable_names!r}."
        )
    if not isinstance(variable.value, list):
        raise StopCliProcess(
            f"Variable {variable_name} should be a list, but it's a {type(variable.value)}"
        )
    console.print(
        f"[bold]Found variable[/bold] {variable.id} with name {variable_name!r}. "
        f"Current compute nodes list has {len(variable.value)} items"
    )
    if settings.DEBUG:
        # This output can be very annoying on large stands like PROD.
        # At the same time it can be very useful for local debugging.
        logging.debug(f"Variable value: {variable.value!r}")
    return cluster, variable


def _get_all_compute_nodes_from_compute(grpc_compute_api_endpoint: str, iam_token: str) -> List[Node]:
    with common.grpc.YcGrpcSecureChannel(grpc_compute_api_endpoint, iam_token) as channel:
        node_service = NodeServiceStub(channel)

        page_token = None
        finished = False
        all_compute_nodes: List[Node] = []
        while not finished:
            result: ListNodesResponse = node_service.List(ListNodesRequest(page_token=page_token))
            all_compute_nodes.extend(result.nodes)
            page_token = result.next_page_token
            if not page_token:
                finished = True

    return all_compute_nodes


def _get_iam_token_from_authorized_key(
    grpc_iam_api_endpoint: str, service_account_id: str, key_id: str, private_key: str
) -> str:
    # See https://cloud.yandex.ru/docs/iam/operations/iam-token/create-for-sa#via-jwt
    now = int(time.time())
    payload = {
        'aud': 'https://iam.api.cloud.yandex.net/iam/v1/tokens',
        'iss': service_account_id,
        'iat': now,
        'exp': now + 360
    }

    encoded_token = jwt.encode(
        payload,
        private_key,
        algorithm='PS256',
        headers={'kid': key_id}
    )

    with common.grpc.YcGrpcSecureChannel(grpc_iam_api_endpoint, iam_token="") as channel:
        iam_token_service = IamTokenServiceStub(channel)
        result: CreateIamTokenResponse = iam_token_service.Create(CreateIamTokenRequest(jwt=encoded_token))
        return result.iam_token


def _get_iam_token_from_authorized_key_file(grpc_iam_api_endpoint: str, authorized_key_file: pathlib.Path) -> str:
    try:
        authorized_key = json.loads(authorized_key_file.read_text())
    except json.JSONDecodeError as e:
        raise StopCliProcess(
            f"Can not parse authorized key passed with --authorized-key-file: {e}",
        ) from e

    if "service_account_id" not in authorized_key:
        raise StopCliProcess(
            f"Authorized key passed via --authorized-key-file doesn't contain \"service_account_id\" field "
            f"(see {authorized_key_file})"
        )

    if "private_key" not in authorized_key:
        raise StopCliProcess(
            f"Authorized key passed via --authorized-key-file doesn't contain \"private_key\" field "
            f"(see {authorized_key_file})"
        )

    if "id" not in authorized_key:
        raise StopCliProcess(
            f"Authorized key passed via --authorized-key-file doesn't contain \"id\" field "
            f"(see {authorized_key_file})"
        )

    service_account_id = authorized_key["service_account_id"]
    key_id = authorized_key["id"]
    private_key = authorized_key["private_key"]

    return _get_iam_token_from_authorized_key(grpc_iam_api_endpoint, service_account_id, key_id, private_key)


@cli.command(
    short_help="receives compute nodes list from Compute Admin API and uploads it to Mr. Prober cluster's variable"
)
@click.option(
    "-c", "--cluster", "cluster_slug", default="meeseeks",
    help="slug of Mr. Prober cluster", show_default=True, type=str
)
@click.option(
    "-v", "--variable", "variable_name", default="compute_nodes",
    help="variable name in Mr. Prober cluster", show_default=True, type=str
)
@click.option(
    "--grpc-iam-api-endpoint", "grpc_iam_api_endpoint", default="ts.private-api.cloud.yandex.net:4282",
    help="grpc endpoint for IAM. Can be obtained in `ycp config list`. "
         "Root certificate path should be in $GRPC_ROOT_CERTIFICATES_PATH",
    show_default=True, type=str,
)
@click.option(
    "--grpc-compute-api-endpoint", "grpc_compute_api_endpoint", default="compute-api.cloud.yandex.net:9051",
    help="grpc endpoint for Compute Admin API. Can be obtained in `ycp config list`. "
         "Root certificate path should be in $GRPC_ROOT_CERTIFICATES_PATH",
    show_default=True, type=str
)
@click.option(
    "--api-endpoint", "api_endpoint", default="https://api.prober.cloud.yandex.net",
    help="URL of Mr. Prober API. Root certificate path should be in $API_ROOT_CERTIFICATES_PATH",
    show_default=True, type=str
)
@click.option(
    "--api-key-file", "api_key_file",
    help="file with API KEY for Mr. Prober API. If not specified, will use $API_KEY", type=str
)
@click.option(
    "--iam-token", "iam_token",
    help="IAM token for making requests to Compute Admin API. "
         "Can be obtained from `yc iam create-token` or from metadata. Alternatively, use --authorized-key-file",
    type=str
)
@click.option(
    "--authorized-key-file", "authorized_key_file",
    help="File with Service Account's authorized key in JSON format for making requests to Compute Admin API. "
         "Alternatively, use --iam-token",
    type=click.Path(exists=True, dir_okay=False, readable=True, resolve_path=True, path_type=pathlib.Path)
)
@click.option(
    "--host-group", "host_group", default="e2e",
    help="host group to filter compute nodes on", show_default=True, type=str
)
@click.option(
    "--compute-node-prefix", "compute_node_prefixes",
    help="list of compute node prefixes, i.e. --compute-node-prefix vla04-s7 --compute-node-prefix sas09-s9",
    required=True, multiple=True, type=str
)
@click.option(
    "--max-amount-of-removed-compute-nodes", "max_amount_of_removed_compute_nodes",
    help="Security threshold which protects the system from unexpected removing of large amount of meeseeks VMs",
    default=10, type=int
)
@click.option("--apply", "apply", is_flag=True, type=bool)
def update_compute_nodes_list(
    cluster_slug: str, variable_name: str, grpc_iam_api_endpoint: str, grpc_compute_api_endpoint: str,
    api_endpoint: str, api_key_file: str,
    iam_token: str, authorized_key_file: pathlib.Path, host_group: str, compute_node_prefixes: Tuple[str],
    max_amount_of_removed_compute_nodes: int, apply: bool,
):
    start_timestamp = int(time.time())

    api_key = read_api_key(api_key_file)
    mr_prober_client = api.client.MrProberApiClient(api_endpoint, api_key=api_key, cache_control="no-store")

    cluster, variable = _find_cluster_and_variable(cluster_slug, mr_prober_client, variable_name)

    if not iam_token:
        if not authorized_key_file:
            report_update_status_metric(start_timestamp, MeeseeksUpdateStatus.INVALID_USAGE)
            raise StopCliProcess(
                "I need IAM-token for making requests to Compute Admin API. Please, pass --iam-token <TOKEN> or "
                "--authorized-key-file <FILE.JSON> with authorized key for the service account.",
                ExitCode.INVALID_ARGUMENTS
            )
        logging.info(f"Trying to exchange authorized key to IAM-token for service account via IAM Private API")
        iam_token = _get_iam_token_from_authorized_key_file(grpc_iam_api_endpoint, authorized_key_file)

    if len(iam_token) > 20:
        # It's long enough to print its parts to debug log
        # See https://bb.yandex-team.ru/projects/CLOUD/repos/iam-access-service-client-java/browse/access-service-client-token/src/main/java/yandex/cloud/iam/sanitizer/IamSanitizerUtil.java
        # for implementation details by IAM.
        logging.debug(f"Using IAM-token: {iam_token[:4]}****{iam_token[-4:]}")

    all_compute_nodes = _get_all_compute_nodes_from_compute(grpc_compute_api_endpoint, iam_token)

    filtered_compute_nodes: List[str] = []
    filter_reason_by_compute_node: Dict[str, str] = {}
    for compute_node in all_compute_nodes:
        is_active_and_enabled = compute_node.status == NodeStatus.ACTIVE and compute_node.enabled
        is_in_host_group = host_group in compute_node.host_groups
        has_one_of_specified_prefix = any(compute_node.name.startswith(prefix) for prefix in compute_node_prefixes)
        if is_active_and_enabled and is_in_host_group and has_one_of_specified_prefix:
            filtered_compute_nodes.append(compute_node.name)
        elif not compute_node.enabled:
            filter_reason_by_compute_node[compute_node.name] = "is not enabled in compute scheduler"
        elif compute_node.status != NodeStatus.ACTIVE:
            filter_reason_by_compute_node[compute_node.name] = f"is non active in compute scheduler " \
                                                               f"(status = {compute_node.status} ≠ {NodeStatus.ACTIVE})"
        elif not is_in_host_group:
            filter_reason_by_compute_node[compute_node.name] = f"is not in host-group {host_group!r}"
        elif not has_one_of_specified_prefix:
            filter_reason_by_compute_node[compute_node.name] = f"has no specified prefix"
        else:
            report_update_status_metric(start_timestamp, MeeseeksUpdateStatus.INTERNAL_ERROR)
            raise StopCliProcess(
                "Possible bug in the code! We decided to remove compute node from the list, but don't know the reason. "
                f"Compute node: {compute_node.name}, it's status in Compute Admin API is {compute_node.status}, "
                f"is it enabled: {compute_node.enabled}, host groups: {compute_node.host_groups}."
            )

    logging.info(f"Total compute nodes count: {len(all_compute_nodes)}")
    logging.info(
        f"Filtered by status (active and enabled), host_group ({host_group!r}) "
        f"and prefixes {compute_node_prefixes!r}: {len(filtered_compute_nodes)}"
    )

    if len(filtered_compute_nodes) == 0:
        report_update_status_metric(start_timestamp, MeeseeksUpdateStatus.INTERNAL_ERROR)
        raise StopCliProcess(
            "List of filtered compute nodes is empty! Probably, something goes wrong. "
            "Didn't update the cluster variable. Check the situation manually!\nRun command with -vv for details."
        )

    if settings.DEBUG:
        # This output can be very annoying on large stands like PROD.
        # At the same time it can be very useful for local debugging.
        logging.debug(f"Filtered compute nodes list: {filtered_compute_nodes}")
    console.print(
        f"[bold]Filtered compute nodes list[/bold] has {len(filtered_compute_nodes)} items"
    )

    if set(variable.value) == set(filtered_compute_nodes):
        report_update_status_metric(start_timestamp, MeeseeksUpdateStatus.EMPTY_DIFF)
        report_meeseeks_count_metric(start_timestamp, len(filtered_compute_nodes))
        print_success_panel(
            "Compute nodes list in Mr. Prober API is equal to compute node list in Cloud Admin API. Nothing to do."
        )
        return

    added_compute_nodes = set(filtered_compute_nodes) - set(variable.value)
    removed_compute_nodes = set(variable.value) - set(filtered_compute_nodes)

    table = Table(
        Column("Will be Added", style="green3", header_style="bold green3"),
        Column("Will be Removed", style="red3", header_style="bold red3"),
        box=box.ROUNDED
    )

    for added, removed in itertools.zip_longest(
            sorted(added_compute_nodes), sorted(removed_compute_nodes), fillvalue=""
    ):
        reason_for_removed_node = filter_reason_by_compute_node.get(removed, "") if removed else ""
        table.add_row(added, f"{removed} [white]{reason_for_removed_node}[/white]")

    console.print(table)

    if not apply:
        print_warning_panel(
            "Nothing will be done, because --apply is not passed. Re-run the command with --apply to apply the changes."
        )
        return

    # Let's protect ourselves from removing too much compute nodes from the list. If it happened, it's a reason to
    # investigate the situation by the team.
    if len(removed_compute_nodes) > max_amount_of_removed_compute_nodes:
        report_update_status_metric(start_timestamp, MeeseeksUpdateStatus.TOO_MUCH_TO_REMOVE)
        raise StopCliProcess(
            f"Too much compute nodes are marked for removal: {len(removed_compute_nodes)}. "
            f"It's allowed to remove not more than {max_amount_of_removed_compute_nodes} "
            "compute nodes at the same time. If you want to increase this limit, specify new threshold value in "
            "--max-amount-of-removed-compute-nodes option."
        )

    mr_prober_client.clusters.update_variable_value(cluster.id, variable.id, filtered_compute_nodes)
    report_update_status_metric(start_timestamp, MeeseeksUpdateStatus.UPDATED)
    report_meeseeks_count_metric(start_timestamp, len(filtered_compute_nodes))

    print_success_panel("List of compute nodes [bold]updated in the API[/bold]")


if __name__ == "__main__":
    main()
