#!/usr/bin/env python3


import argparse


def filter_node_cli():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-bgp",
        "--bgp_daemon_configuration_file_path",
        help="name of bgp daemon configuration file",
        #        default = "bird.conf.static_routes",
        type=str,
    )
    parser.add_argument(
        "-suricata",
        "--suricata_rules_file_path",
        help="name of suricata signatures configuration file",
        #        default = "react.rules",
        type=str,
    )
    parser.add_argument(
        "-s3",
        "--s3_endpoint",
        help="Domain name/ip address of S3 API endpoint",
        type=str,
    )
    parser.add_argument("-b", "--s3_bucket_name", help="S3 bucket name", type=str)
    parser.add_argument(
        "-i", "--s3_auth_key_id", help="S3 authentication key ID", type=str
    )
    parser.add_argument(
        "-s", "--s3_secret_auth_key", help="S3 secret authentication key", type=str
    )
    parser.add_argument(
        "-p",
        "--s3_object_key_prefix",
        help="S3 object key prefix for actual dump",
        type=str,
    )
    parser.add_argument(
        "-cf",
        "--config_file",
        help="This utility configuration file location",
        default="/etc/yc/rkn/filter_node.yaml",
        type=str,
    )
    parser.add_argument(
        "-a",
        "--auth_file",
        help="This utility configuration file location",
        default="/etc/yc/rkn/auth.yaml",
        type=str,
    )
    parser.add_argument(
        "-l",
        "--logfile",
        help="logfile",
        default="/var/log/yc_rkn_filter_node.log",
        type=str,
    )
    parser.add_argument(
        "-q",
        "--quiet",
        help="do not print to stdout",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        help="print DEBUG to stdout",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-pid",
        "--pid_file_path",
        help="path to pid file",
        default="/run/yc-rkn-yc-rkn-filter-node",
        type=str,
    )
    parser.add_argument(
        "-fb",
        "--fallback_files_locations",
        help="files pathes which should be used as last resort in case of S3 communication failure",
        #        default = "/etc/yc/rkn/friendly_nets.yaml",
        nargs="*",
        type=str,
    )
    cli_input = parser.parse_args()
    return cli_input
