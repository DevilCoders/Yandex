#!/usr/bin/env python3

import argparse
import logging
import random

from yaqmetrics import MetricsQueryGroupClient


def parse_args():
    parser = argparse.ArgumentParser(description="add random label to basket")
    parser.add_argument(
        "--token",
        required=True,
        help="oauth token",
    )
    parser.add_argument(
        "--basket",
        required=True,
        help="basket id",
    )
    return parser.parse_args()


def main():
    logging.basicConfig(format="[%(levelname)s] %(asctime)s - %(message)s", level=logging.INFO)
    cli_args = parse_args()
    token = cli_args.token
    basket_id = cli_args.basket

    client = MetricsQueryGroupClient(token)
    queries = client.fetch_query_basket(basket_id)

    for q in queries:
        labels = q.setdefault("labels", [])
        if "part_1_" not in labels and "part_2_" not in labels:
            if "part_1" in labels:
                labels.append("part_1_" + str(random.randint(1, 3)))
            if "part_2" in labels:
                labels.append("part_2_" + str(random.randint(1, 3)))

    client.put_queries(basket_id, queries)


if __name__ == "__main__":
    main()
