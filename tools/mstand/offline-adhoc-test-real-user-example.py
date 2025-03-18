#!/usr/bin/env python3

import argparse

from vault_client.instances import Production as VaultClient

import yaqutils.args_helpers as uargs
import yaqutils.misc_helpers as umisc
import yaqutils.json_helpers as ujson

import serp.mstand_offline_adhoc as mstand_adhoc


def parse_args():
    parser = argparse.ArgumentParser(description="Test offline adhoc API for debuging metrics (SUPERAPP-1377)")
    uargs.add_generic_params(parser)

    return parser.parse_args()


def get_token_from_yav(token_id, token_key):
    client = VaultClient(decode_files=True)
    head_version = client.get_version(token_id)
    return head_version['value'][token_key]


def main_worker(cli_args):
    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)
    # metric_ctx = mstand_adhoc.load_metric(class_name="MetricaQueryDocShare", module_name="metrica_fullness")

    serp_attrs = mstand_adhoc.make_serp_attrs(
        component_reqs=['componentInfo'],
        judgements=["albin_host_bundle_old"],
        serp_reqs=['texts.labels'],
    )

    from yaqmetrics import MetricsSerpsetsClient

    token = get_token_from_yav('sec-01g0fffypapkd8509skrr1d0dq', 'token')
    client = MetricsSerpsetsClient(token)
    serpset_ids = [37455370, 37439254]

    for serpset_id in serpset_ids:
        test_serpset_tiny = client.get_serpset_inplace(
            serpset_id,
            serpset_filter='true',
            pre_filter='query-485225',
            requirement=list(serp_attrs.requirement_set),
            strict=True
        )

        json_file_name = "work_dir/{}_serpset_tiny.json".format(serpset_id)
        ujson.dump_to_file(test_serpset_tiny, json_file_name)

        parsed_serps = mstand_adhoc.load_serpset(
            json_file_name,
            serp_attrs=serp_attrs,
            allow_no_position=True,
            allow_broken_components=True,
        )

        raise Exception("stop here")

        # for parsed_serp in parsed_serps:
        #     smv = mstand_adhoc.compute_metric_on_serp(metric_ctx=metric_ctx, parsed_serp=parsed_serp)
        #     for label in parsed_serp.markup_info.serp_data["texts.labels"]:
        #         pass
        #         # yt_client.write_table_structured(
        #         #     table_path,
        #         #     Row,
        #         #     [
        #         #         Row(
        #         #             serpset_id=serpset_id,
        #         #             label=label,
        #         #             has_top_url=smv[0],
        #         #             has_ad_or_sr_result_type=smv[1],
        #         #             has_metrica=smv[2],
        #         #         )
        #         #     ]
        #         # )
        #         # spamwriter.writerow([serpset_id, label, *smv.get_total_value()])


def main():
    cli_args = parse_args()
    main_worker(cli_args)


if __name__ == "__main__":
    main()
