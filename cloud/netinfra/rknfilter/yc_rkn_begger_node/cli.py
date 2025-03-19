#!/usr/bin/env python3


import argparse


def begger_node_cli():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "-d", "--dump_file_path", help="RKN dump file location", type=str
    )
    parser.add_argument(
        "-o", "--operator_name", help="YaTelecom operator name", type=str
    )
    parser.add_argument("-inn", "--inn", help="Inn name", type=str)
    parser.add_argument("-og", "--ogrn", help="ogrn", type=str)
    parser.add_argument("-e", "--email", help="e-mail of responcible person", type=str)
    parser.add_argument(
        "-pr",
        "--plain_text_request_file_path",
        help="path to plain text request file",
        type=str,
    )
    parser.add_argument(
        "-ds",
        "--digital_signature_file_path",
        help="path to signed request file",
        type=str,
    )
    parser.add_argument("-k", "--key_file_path", help="path to key file", type=str)
    parser.add_argument(
        "-c", "--certificate_file_path", help="path to certificate file", type=str
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
        default="/etc/yc/rkn/begger_node.yaml",
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
        "-ru",
        "--rkn_url",
        help="rkn API url",
        #        default = "/etc/yc/rkn/auth.yaml",
        type=str,
    )
    parser.add_argument(
        "-F",
        "--force_downloading_process",
        help="run downloding process despite current state",
        #        default = "/etc/yc/rkn/auth.yaml",
        action="store_true",
    )
    parser.add_argument(
        "-l",
        "--logfile",
        help="logfile",
        default="/var/log/yc_rkn_begger_node.log",
        type=str,
    )
    parser.add_argument(
        "-pid",
        "--pid_file_path",
        help="path to pid file",
        default="/run/yc-rkn-begger-node",
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
    cli_input = parser.parse_args()
    return cli_input


def begger_init_cli():
    parser = argparse.ArgumentParser()
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
        "-vp",
        "--s3_vipnet_object_key_prefix",
        help="S3 object key prefix for actual dump",
        type=str,
    )
    parser.add_argument(
        "-cp",
        "--chroot_path",
        help="path to deploy chroot with viptella deb's",
        type=str,
    )
    parser.add_argument(
        "-va",
        "--vipnet_archive_file_path",
        help="path to deploy chroot with viptella deb's",
        type=str,
    )
    parser.add_argument(
        "-cb",
        "--chroot_content_backup_file_path",
        help="path to chroot content backup file",
        type=str,
    )
    parser.add_argument(
        "-l",
        "--logfile",
        help="logfile",
        default="/var/log/yc_rkn_begger_init.log",
        type=str,
    )
    parser.add_argument(
        "-cf",
        "--config_file",
        help="This utility configuration file location",
        default="/etc/yc/rkn/begger_node.yaml",
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
        "-pid",
        "--pid_file_path",
        help="path to pid file",
        default="/run/yc-rkn-begger-init",
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
    cli_input = parser.parse_args()
    return cli_input
