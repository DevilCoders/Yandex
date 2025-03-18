#!/usr/bin/env python3

import argparse
import requests

import experiment_pool.pool_helpers as pool_helpers
import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc


def parse_args():
    parser = argparse.ArgumentParser(description="Adv density analysis tool")
    uargs.add_verbosity(parser)
    mstand_uargs.add_input_pool(parser)

    return parser.parse_args()


def get_serpset_info(serpset_id):
    params = {
        "id": serpset_id
    }
    resp = requests.get("https://ssc.metrics.yandex-team.ru/api/serpset/info", params=params, verify=False)
    assert resp.ok
    serpset_info = resp.json()
    serpset_name = serpset_info.get("name")
    return serpset_name


def print_header():
    print("#|")
    print("|| HasAdv | AdvCount | AdvExpCount | serpset | details ||")


def print_footer():
    print("|#")


def main():
    cli_args = parse_args()
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    pool = pool_helpers.load_pool(cli_args.input_file)

    results = []
    for exp in pool.all_experiments():
        has_adv = exp.metric_results[0].metric_values.significant_value
        adv_count = exp.metric_results[1].metric_values.significant_value
        adv_exp_count = exp.metric_results[2].metric_values.significant_value
        serpset_name = get_serpset_info(exp.serpset_id)
        params = "&regional=RU&evaluation=WEB&aspect=default&serpset-filter=addvOrder"
        serpset_url = "https://metrics.yandex-team.ru/mc/compare2?serpset={}{}".format(exp.serpset_id, params)
        results.append((adv_count, has_adv, adv_exp_count, serpset_url, exp.serpset_id, serpset_name))

    # sorted by adv count
    print_header()
    basket_switched = False

    for row in sorted(results):

        adv_count, has_adv, adv_exp_count, serpset_url, serpset_id, serpset_name = row
        if not basket_switched and adv_count > 0.3:
            # separate commercial basket
            basket_switched = True
            print_footer()
            print_header()

        print("|| {:.4f} | {:.4f} | {:.2f} | {} | (({} {}))||".format(has_adv, adv_count, adv_exp_count, serpset_name, serpset_url, serpset_id))
    print_footer()


if __name__ == "__main__":
    main()
