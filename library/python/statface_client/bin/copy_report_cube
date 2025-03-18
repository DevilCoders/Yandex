#!/usr/bin/env python
# coding: utf8

from __future__ import division, absolute_import, print_function  # , unicode_literals

import argparse
# import subprocess

from yt.wrapper.client import YtClient
import yt.transfer_manager.client as tm

import statface_client
import statface_client.constants


def run_dyn_to_static(cluster, src, dst):
    client = YtClient(proxy=cluster)
    with client.Transaction():  # pylint: disable=no-member
        # Take snapshot lock for source table.
        client.lock(src, mode="snapshot")  # pylint: disable=no-member
        # Get timestamp of flushed data.
        data_timestamp = client.get_attribute(  # pylint: disable=no-member
            src, "unflushed_timestamp") - 1
        # Create table and save vital attributes.
        client.remove(dst)  # pylint: disable=no-member
        print("Create dst table {} on cluster {}".format(dst, cluster))
        client.create(  # pylint: disable=no-member
            'table',
            path=dst,
            attributes={
                "optimize_for": client.get_attribute(  # pylint: disable=no-member
                    src, "optimize_for", default="lookup"),
                "schema": client.get_attribute(  # pylint: disable=no-member
                    src, "schema"),
                "_yt_dump_restore_pivot_keys": client.get_attribute(  # pylint: disable=no-member
                    src, "pivot_keys"),
            })
        # Dump table contents.
        print("Run merge from {} to {} on cluster {}".format(src, dst, cluster))
        client.run_merge(  # pylint: disable=no-member
            client.TablePath(  # pylint: disable=no-member
                src, attributes={"timestamp": data_timestamp}),
            dst, mode="ordered")


def get_cube_dynamic_table_path(
        report,
        scale,
        host=statface_client.STATFACE_PRODUCTION):

    client = statface_client.StatfaceClient(host=host)
    report = client.get_report(report)
    response = report._api._report_get(
        uri='config',
        params=dict(verbose=1)
    )
    answer = response.json()
    for scale_info in answer['scales_info']:
        if scale_info['available'] and scale_info['short_name'] == scale:
            return scale_info['table_name']
    raise KeyError('There is no scale "%s" in the report "%s"' % (scale, report.path))


def get_temp_table_path(cluster):
    client = YtClient(proxy=cluster)
    return client.create_temp_table()  # pylint: disable=no-member


def download_cube(report_path, report_proxy, scale, dst_yt_proxy, dst_path):

    print(("download cube for path={path} proxy={proxy} scale={scale}"
           " yt-proxy={yt_proxy} yt-path={yt_path}").format(
               path=report_path, proxy=report_proxy, scale=scale,
               yt_proxy=dst_yt_proxy, yt_path=dst_path))
    report_cube_dyn_path = get_cube_dynamic_table_path(
        report=report_path, scale=scale, host=report_proxy)
    cube_cluster = REPORT_CLUSTERS[report_proxy]
    tmp_static_path = get_temp_table_path(cube_cluster)

    run_dyn_to_static(
        cube_cluster,
        report_cube_dyn_path,
        tmp_static_path
    )

    return tm.add_task(
        source_cluster=cube_cluster,
        source_table=tmp_static_path,
        destination_cluster=dst_yt_proxy,
        destination_table=dst_path,
        sync=True)


REPORT_PROXIES = {
    'stat': statface_client.STATFACE_PRODUCTION,
    'stat-beta': statface_client.STATFACE_BETA}
REPORT_CLUSTERS = {
    'stat': 'vanga',
    'stat-beta': 'pythia'}
REPORT_CLUSTERS.update({
    proxy_addr: REPORT_CLUSTERS[proxy_name]
    for proxy_name, proxy_addr in REPORT_PROXIES.items()})


def expand_proxy_name(short_name):
    return REPORT_PROXIES[short_name]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('report')
    parser.add_argument('dstpath')

    parser.add_argument('-rp', '--report-proxy', choices=['stat', 'stat-beta'],
                        default='stat')
    parser.add_argument('-s', '--scale',
                        choices=statface_client.constants.STATFACE_SCALES,
                        default=statface_client.constants.DAILY_SCALE)
    parser.add_argument('-d', '--dst-yt-proxy', default='hahn')

    args = parser.parse_args()

    report_proxy_name = args.report_proxy
    report_proxy_addr = expand_proxy_name(report_proxy_name)

    return download_cube(
        report_proxy=report_proxy_addr,
        report_path=args.report, scale=args.scale,
        dst_yt_proxy=args.dst_yt_proxy, dst_path=args.dstpath)


if __name__ == "__main__":
    main()
