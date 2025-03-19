#!/usr/bin/env python3

import argparse
import subprocess
import yaml
import shutil


def binary_exists(name):
    if not shutil.which(name):
        raise BaseException(f'{name} must be in PATH')


def call(cmd):
    print(f'Executing {cmd}')
    res = subprocess.check_output([cmd], universal_newlines=True, shell=True, stderr=subprocess.STDOUT)
    print(res)
    return res


def call_yc(cmd, args):
    return call(f'yc --profile {args.yc_profile} {cmd}')


def call_yc_lb(cmd, args):
    return call(f'yes | logbroker -s {args.yc_lb_server} {cmd}')


def call_yandex_lb(cmd, args):
    return call(f'yes | logbroker -s {args.yandex_lb_server} {cmd}')


def load_sa(name, args):
    try:
        return call_yc(f'iam service-account get --name {name}', args)
    except:
        return call_yc(f'iam service-account create --name {name}', args)


def create_sa(name, args):
    res = load_sa(name, args)
    dict_res = yaml.load(res, yaml.SafeLoader)
    return dict_res['id']


def yc_lb_mirrorer_sa(args):
    return create_sa('logbroker-yandex-mirrorer', args)


def link_lbs(args):
    yc_lb_mirrorer_sa_id = yc_lb_mirrorer_sa(args)
    yc_lb_producer_sa_id = create_sa(args.yc_lb_producer_sa, args)

    # non-repeatable
    call_yc_lb(f'schema create topic -p {args.lb_partitions} {args.yc_lb_topic}', args)

    call_yc(f'iam key create --service-account-id {yc_lb_producer_sa_id} -o lb_producer_key.json', args)
    call_yc_lb(f'permissions grant --path {args.yc_lb_topic} --subject {yc_lb_producer_sa_id}@as --permissions WriteTopic', args)

    # non-repeatable
    call_yandex_lb(f'schema create topic -p {args.lb_partitions} {args.yandex_lb_mirrored_topic}', args)
    # non-repeatable
    call_yandex_lb(f'yt-delivery add --topic {args.yandex_lb_mirrored_topic} --yt hahn', args)

    call_yc(f'iam key create --service-account-id {yc_lb_mirrorer_sa_id} -o logbroker-mirrorer-key.json', args)
    call_yc_lb(f'permissions grant --path {args.yc_lb_topic} --subject {yc_lb_mirrorer_sa_id}@as --permissions ReadTopic', args)
    # non-repeatable
    call_yc_lb(f'schema create read-rule -t {args.yc_lb_topic} -c shared/remote-mirror --all-original', args)

    # non-repeatable
    call(f'yes | logbroker -s {args.yandex_lb_server} schema create remote-mirror-rule -t {args.yandex_lb_mirrored_topic} -c sas --src-cluster {args.yc_lb_server} --src-topic {args.yc_lb_topic} --src-consumer shared/remote-mirror --iam-key-file ./logbroker-mirrorer-key.json')


def check_yc_lb_account(args):
    call_yc_lb(f'schema list /{args.yc_cloud_id}', args)
    print('If no account found you must create one - https://wiki.yandex-team.ru/logbroker/dev/yc-lb/how-to-use-lb-in-yc/')


def create_yc_lb_cp_directory(args):
    call_yc_lb(f'schema create directory /{args.yc_cloud_id}/controlplane', args)


def main():
    binary_exists('yc')
    binary_exists('logbroker')

    parser = argparse.ArgumentParser(description='Setup compute logbroker to deliver logs to YT.')
    parser.add_argument('--yc-profile', required=True, help='YC profile to use')
    subparsers = parser.add_subparsers(help='subparsers')

    parser_check_yc_lb_account = subparsers.add_parser('check-yc-lb-account')
    parser_check_yc_lb_account.add_argument('--yc-lb-server', required=True, help='LB server @ YC')
    parser_check_yc_lb_account.add_argument('--yc-cloud-id', required=True, help='Cloud ID for YC')
    parser_check_yc_lb_account.set_defaults(func=check_yc_lb_account)

    parser_create_yc_lb_cp_directory = subparsers.add_parser('create-yc-lb-cp-directory')
    parser_create_yc_lb_cp_directory.add_argument('--yc-lb-server', required=True, help='LB server @ YC')
    parser_create_yc_lb_cp_directory.add_argument('--yc-cloud-id', required=True, help='Cloud ID for YC')
    parser_create_yc_lb_cp_directory.set_defaults(func=create_yc_lb_cp_directory)

    parser_link_lbs = subparsers.add_parser('link-lbs')
    parser_link_lbs.add_argument('--yc-lb-server', required=True, help='LB server @ YC')
    parser_link_lbs.add_argument('--yandex-lb-server', required=True, help='LB server @ Yandex')
    parser_link_lbs.add_argument('--yc-lb-producer-sa', required=True, help='SA name for LB producer')
    parser_link_lbs.add_argument('--yc-token', required=True, help='OAuth token for LB configuration')
    parser_link_lbs.add_argument('--lb-partitions', required=True, help='LB partition count for topic')
    parser_link_lbs.add_argument('--yc-lb-topic', required=True, help='LB topic name @ YC')
    parser_link_lbs.add_argument('--yandex-lb-mirrored-topic', required=True, help='LB mirrored topic name @ Yandex')
    parser_link_lbs.set_defaults(func=link_lbs)

    args = parser.parse_args()

    try:
        args.func(args)
    except subprocess.CalledProcessError as ex:
        print(f'Command error:\n{ex.cmd}\n{ex.stdout}')


if __name__ == '__main__':
    main()
