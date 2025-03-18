# -*- coding: utf-8 -*-
import requests
import argparse
import time
import ticket_parser2.api.v1 as tp2


def call_cbb(url, params, tvm_ticket):
    return requests.get(url, params, headers={"X-Ya-Service-Ticket": tvm_ticket})


def parse_args():
    parser = argparse.ArgumentParser()
    # Свой ID можно посмотреть в ресурсах сервиса (тип "TVM приложение")
    # там же можно взять секрет
    parser.add_argument("--self-tvm-client-id", required=True, type=int, help="Self TVM client id")
    parser.add_argument("--tvm-secret", required=True, help="TVM secret")
    return parser.parse_args()


def main():
    args = parse_args()

    flag = 497

    api_address = "https://cbb.n.yandex-team.ru"
    cbb_tvm_client_id = 2000300

    tvm_client = tp2.TvmClient(tp2.TvmApiClientSettings(
        self_client_id=args.self_tvm_client_id,
        self_secret=args.tvm_secret,
        dsts={"cbb": cbb_tvm_client_id}
    ))

    with open("/home/tyamgin/tmp/aa/cbb.sql") as inp:
        for line in inp.readlines():
            line = line.strip()
            if not line:
                continue

            parts = line.split("\t")
            if parts[0] == str(flag) and len(parts) == 7:
                _, dt, desc, expire, user, _, reg = parts
                if 'TVM:2025676' in user and 'automatically generated' in desc and expire != "\\N" and expire >= '2021-02-30 00:00:00' and expire < '2021-03-03 00:00:00':
                    reg = reg.replace('\\\\', '\\')
                    print(f"Remove: {reg}")

                    continue

                    resp = call_cbb(api_address + "/api/v1/set_range", {
                        "operation": "del",
                        "range_txt": reg,
                        "flag": str(flag),
                    }, tvm_client.get_service_ticket_for("cbb"))
                    print(f"Status code {resp.status_code}")
                    print(resp.content.decode())
                    print("")
                    time.sleep(0.1)


if __name__ == '__main__':
    main()
