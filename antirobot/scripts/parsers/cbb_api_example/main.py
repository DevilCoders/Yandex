# -*- coding: utf-8 -*-
import requests
import argparse
import datetime
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

    api_address = "https://cbb-testing.n.yandex-team.ru"  # для прода: https://cbb-ext.yandex-team.ru
    cbb_tvm_client_id = 2002238  # для прода: 2000300
    flag = 185  # разметка DDOS

    tvm_client = tp2.TvmClient(tp2.TvmApiClientSettings(
        self_client_id=args.self_tvm_client_id,
        self_secret=args.tvm_secret,
        dsts={"cbb": cbb_tvm_client_id}
    ))

    resp = call_cbb(api_address + "/api/v1/set_range", {
        "operation": "add",
        "range_txt": "header['Host']=/ya.ru/;header['User-Agent']=/.*Keenetic.*/",
        "flag": str(flag),
        "description": "Тестируем добавление через API",
        "expire": (datetime.datetime.now() + datetime.timedelta(days=30)).strftime("%s"),  # если отсутствует, то используется дефолтный expire группы
    }, tvm_client.get_service_ticket_for("cbb"))

    print(f"Status code {resp.status_code}")
    print(resp.content.decode())


if __name__ == '__main__':
    main()
