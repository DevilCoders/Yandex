#!/usr/bin/env python3


import argparse


def config_node_cli():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-d", "--dump_file_path", help="RKN dump files locations", type=str
    )
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
        "-t",
        "--reconfiguration_triggering_threshholds",
        help="Definition of acceptable amount ranges during configuration process",
        type=str,
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
        default="/etc/yc/rkn/config_node.yaml",
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
        "-bw",
        "--bgp_asn_whitelist_files_locations",
        help="whitelist asns files_locations",
        nargs="*",
        #        default = "/etc/yc/rkn/whitelist_asns.yaml",
        type=str,
    )
    parser.add_argument(
        "-bd",
        "--bgp_asn_diamondlist_files_locations",
        help="diamondlist asns files_locations",
        #        default = "/etc/yc/rkn/whitelist_asns.yaml",
        nargs="*",
        type=str,
    )
    parser.add_argument(
        "-f",
        "--friendly_nets_files_locations",
        help="friendly nets files_locations",
        #        default = "/etc/yc/rkn/friendly_nets.yaml",
        nargs="*",
        type=str,
    )
    parser.add_argument(
        "-dw",
        "--domain_name_whitelist_files_locations",
        help="white lists files_locations",
        #        default = "/etc/yc/rkn/friendly_nets.yaml",
        nargs="*",
        type=str,
    )
    parser.add_argument(
        "-ra",
        "--dns_resolve_attempts",
        help="dns_resolution attemps",
        #        default = "/etc/yc/rkn/friendly_nets.yaml",
        type=int,
    )
    parser.add_argument(
        "-F",
        "--force_configuration_process",
        help="run configuration process despite dump files were not updated",
        #        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-ed",
        "--s3_object_expiration_days",
        help="define expiration days for s3 objects ",
        #        default=14,
        type=int,
    )
    parser.add_argument(
        "-eh",
        "--s3_object_expiration_hours",
        help="define expiration hours for s3 objects ",
        #        default=0,
        type=int,
    )
    parser.add_argument(
        "-em",
        "--s3_object_expiration_minutes",
        help="define expiration minutes for s3 objects ",
        #        default=0,
        type=int,
    )
    parser.add_argument(
        "-r",
        "--use_resolved_addresses_only",
        help=" ignore addressing information from RKN dump file for L7 rules and use only resolved",
        #        default=0,
        type=bool,
    )
    parser.add_argument(
        "-l",
        "--logfile",
        help="logfile",
        default="/var/log/yc_rkn_config_node.log",
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
        help="enable verbose logging",
        default=False,
        action="store_true",
    )
    parser.add_argument(
        "-pid",
        "--pid_file_path",
        help="path to pid file",
        default="/run/yc-rkn-config-node",
        type=str,
    )
    parser.add_argument(
        "-dr",
        "--dryrun",
        help="run program in dry-run mode",
        default=False,
        action="store_true",
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
