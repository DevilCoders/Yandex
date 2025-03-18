# -*- coding: utf-8 -*-
import requests
import argparse
import logging
import sys
import ticket_parser2.api.v1 as tp2
from retry import retry


logger = logging.getLogger(__name__)
handler = logging.StreamHandler(stream=sys.stderr)
logger.addHandler(handler)
logger.setLevel(logging.DEBUG)


@retry(requests.exceptions.RequestException, tries=5, delay=20)
def call_cbb(url, params, tvm_ticket):
    return requests.post(url, headers={"X-Ya-Service-Ticket": tvm_ticket}, data=params)


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument("--api-address", required=True, help="e.x. https://cbb-testing.n.yandex-team.ru")
    parser.add_argument("--input", required=True, help="Input json file")
    parser.add_argument("--flag", required=True, type=int, help="Flag id")
    parser.add_argument("--self-tvm-client-id", required=True, type=int, help="Self TVM client id")
    parser.add_argument("--cbb-tvm-client-id", required=True, type=int, help="CBB TVM client id")
    parser.add_argument("--tvm-secret", required=True, help="TVM secret")
    return parser.parse_args()


def main():
    args = parse_args()

    tvm_client = tp2.TvmClient(tp2.TvmApiClientSettings(
        self_client_id=args.self_tvm_client_id,
        self_secret=args.tvm_secret,
        dsts={"cbb": args.cbb_tvm_client_id}
    ))

    with open(args.input) as f:
        ips = f.read()
        resp = call_cbb(args.api_address + "/api/v1/set_ranges", {
            "flag": str(args.flag),
            "rng_list": ips,
            "description": "bulk add",
        }, tvm_client.get_service_ticket_for("cbb"))

        if resp.status_code != requests.codes.ok:
            logger.error(f"Status code {resp.status_code}")
            logger.error(resp.content.decode())
        else:
            logger.debug(f"Status code {resp.status_code}")
            logger.debug(resp.content.decode())


if __name__ == '__main__':
    main()
