#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import argparse
import yaqutils.nirvana_helpers as unirv
import cli_tools.squeeze_yt_single as runner


def main():
    parser = argparse.ArgumentParser(description="Collect data from user_sessions into MR table for mstand")
    runner.add_arguments(parser)
    unirv.run_and_save_exception(runner.main, parser.parse_args())


if __name__ == "__main__":
    main()
