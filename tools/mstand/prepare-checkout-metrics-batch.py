#!/usr/bin/env python3

import argparse
import logging
import os

import mstand_utils.args_helpers as mstand_uargs
import yaqutils.args_helpers as uargs
import yaqutils.six_helpers as usix
import mstand_utils.checkout_helpers as ucheckout
import yaqutils.json_helpers as ujson
import yaqutils.misc_helpers as umisc
from user_plugins import PluginBatch


def parse_args():
    parser = argparse.ArgumentParser(description="svn batch checkout utility")
    mstand_uargs.add_plugin_params(parser)
    mstand_uargs.add_output_json(parser, required=True)
    uargs.add_verbosity(parser)
    parser.add_argument("--checkout-batch", help="write checkout batch json to this file", required=True)
    parser.add_argument("--batch-meta-info", help="write batch meta info json to this file")

    return parser.parse_args()


def prepare_path_to_checkout(metric_module_url: str) -> str:
    metric_module_path = ucheckout.truncate_url(metric_module_url)
    # METRICS-8231: cutoff filename, if any
    if metric_module_path.endswith(".py"):
        logging.warning("path '%s' leads to metric file, not a module", metric_module_path)
        metric_module_path = os.path.dirname(metric_module_path)
        logging.warning("autofixed metric module path: '%s'", metric_module_path)

    return metric_module_path


def main():
    cli_args = parse_args()

    assert cli_args.batch, "Batch must be set"
    assert cli_args.output_json, "Output json must be set"
    assert cli_args.checkout_batch, "Checkout batch must be set"

    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    batch_file = cli_args.batch
    plugin_batch = PluginBatch.load_from_file(batch_file)
    logging.info("got %d plugin sources", len(plugin_batch.plugin_sources))
    module_urls = {}
    batch_items = {}

    # number of metrics to calculate
    metrics_number = 0
    for plugin_source in plugin_batch.plugin_sources:
        metrics_number += len(plugin_source.kwargs_list)
        logging.info("handle %s", plugin_source)
        assert plugin_source.url, "URL must be set"
        module_name = ucheckout.generate_module_name(plugin_source)

        revision_url = plugin_source.url + "@" + str(plugin_source.revision)
        if module_name in module_urls:
            assert module_urls[module_name] == revision_url, "Module names clashing"
        module_urls[module_name] = revision_url

        path_to_checkout = prepare_path_to_checkout(plugin_source.url)

        checkout_entry = {
            "path": path_to_checkout,
            "rev": int(plugin_source.revision),
            "dir": module_name
        }
        if module_name in batch_items:
            assert checkout_entry == batch_items[module_name]
        else:
            batch_items[module_name] = checkout_entry
        plugin_source.module_name = module_name + "." + plugin_source.module_name

    batch_items_sorted = sorted(usix.itervalues(batch_items), key=lambda e: e["dir"])
    logging.info("will checkout %d unique paths:", len(batch_items_sorted))
    for checkout_entry in batch_items_sorted:
        logging.info("%s: %s@%s", checkout_entry["dir"], checkout_entry["path"], checkout_entry["rev"])

    checkout_batch = {
        "batch": batch_items_sorted,
    }

    batch_meta_info = {
        "metrics-number": metrics_number
    }
    plugins = plugin_batch.serialize()
    ujson.dump_to_file(plugins, cli_args.output_json)
    ujson.dump_to_file(checkout_batch, cli_args.checkout_batch)
    if cli_args.batch_meta_info:
        ujson.dump_to_file(batch_meta_info, cli_args.batch_meta_info)


if __name__ == "__main__":
    main()
