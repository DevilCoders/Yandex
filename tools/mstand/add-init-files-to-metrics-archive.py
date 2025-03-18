#!/usr/bin/env python3

import argparse
import logging
import os

import yaqutils.args_helpers as uargs
import yaqutils.file_helpers as ufile
import yaqutils.misc_helpers as umisc

import mstand_utils.args_helpers as mstand_uargs
import mstand_utils.checkout_helpers as ucheckout
from user_plugins import PluginBatch


def parse_args():
    parser = argparse.ArgumentParser(description="svn batch checkout utility")
    mstand_uargs.add_plugin_params(parser)
    mstand_uargs.add_save_to_tar(parser)
    uargs.add_verbosity(parser)
    return parser.parse_args()


def main():
    cli_args = parse_args()

    assert cli_args.batch, "Batch must be set"
    assert cli_args.source, "Source must be set"
    assert cli_args.save_to_tar, "Save to tar must be set"

    umisc.configure_logger(verbose=cli_args.verbose, quiet=cli_args.quiet)

    metrics_dir = ufile.extract_tar_to_temp(cli_args.source)
    plugin_batch = PluginBatch.load_from_file(cli_args.batch)
    for plugin_source in plugin_batch.plugin_sources:
        module = ucheckout.generate_module_name(plugin_source)
        module_dir = os.path.join(metrics_dir, module)
        if not os.path.exists(module_dir):
            logging.info("ignore missed dir: %s", module_dir)
        else:
            init_py_file = os.path.join(module_dir, "__init__.py")
            if not os.path.exists(init_py_file):
                logging.info("create empty file %s", init_py_file)
                ufile.write_text_file(init_py_file, "")
            logging.info("removing .svn metainfo directory")
            svn_meta_dir = os.path.join(module_dir, ".svn")
            ufile.remove_if_exists(svn_meta_dir)

    ufile.tar_directory(metrics_dir, cli_args.save_to_tar, True)


if __name__ == "__main__":
    main()
