import grpc

import yandex.cloud.priv.smartcaptcha.v1.captcha_service_pb2_grpc as api_pb2_grpc
import yandex.cloud.priv.smartcaptcha.v1.captcha_service_pb2 as api_pb2
import yandex.cloud.priv.smartcaptcha.v1.captcha_pb2 as captcha_pb2

import argparse
import sys


"""
USAGE:
./cloud_api_tool --endpoint captcha-cloud-api.yandex-team.ru:80 create --captcha_id junk_captcha_1 --cloud_id junk_cloud --folder_id junk_folder --name my_junk_captcha --allowed_sites example.com --allowed_sites example.ru
./cloud_api_tool --endpoint captcha-cloud-api.yandex-team.ru:80 get --captcha_id junk_captcha_1
"""  # noqa: E501


def main():
    args = parse_args()

    if args.command == "create":
        return create_func(args)
    if args.command == "get":
        return get_func(args)
    else:
        raise Exception(f"Unknown command {args.command}")


def create_func(args):
    channel = grpc.insecure_channel(args.endpoint)
    stub = api_pb2_grpc.CaptchaSettingsServiceStub(channel)
    new_captcha_op = stub.Create(api_pb2.CreateCaptchaRequest(
        cloud_id=args.cloud_id,
        folder_id=args.folder_id,
        name=args.name,
        allowed_sites=args.allowed_sites,
    ))

    new_captcha = captcha_pb2.CaptchaSettings()
    new_captcha_op.response.Unpack(new_captcha)
    print(new_captcha)


def get_func(args):
    channel = grpc.insecure_channel(args.endpoint)
    stub = api_pb2_grpc.CaptchaSettingsServiceStub(channel)
    res = stub.Get(api_pb2.GetSettingsRequest(
        captcha_id=args.captcha_id
    ))
    print(res)


def parse_args():
    parser = argparse.ArgumentParser(
        description="Cloud captcha API",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    parser.add_argument("--endpoint", required=True, type=str, help="Cloud API endpoint")

    subparsers = parser.add_subparsers(dest="command")
    add_parse_create_cmd(subparsers)
    add_parse_get_cmd(subparsers)

    return parser.parse_args()


def add_parse_create_cmd(subparsers):
    sp = subparsers.add_parser("create", help="create")

    sp.add_argument("--captcha_id", required=True, type=str, help="captcha_id")
    sp.add_argument("--cloud_id", required=True, type=str, help="cloud_id")
    sp.add_argument("--folder_id", required=True, type=str, help="folder_id")
    sp.add_argument("--name", required=True, type=str, help="name")
    sp.add_argument("--allowed_sites", required=True, action="append", type=str, help="allowed_sites")


def add_parse_get_cmd(subparsers):
    sp = subparsers.add_parser("get", help="get")

    sp.add_argument("--captcha_id", required=True, type=str, help="captcha_id")


if __name__ == "__main__":
    sys.exit(main() or 0)
