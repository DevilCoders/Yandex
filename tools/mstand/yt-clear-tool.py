#!/usr/bin/env python3

import cli_tools.yt_clear_tool as yt_clear_tool
import pytlib.client_yt as ytcli
import yaqutils.misc_helpers as umisc


def main() -> None:
    umisc.configure_logger()
    cli_args = yt_clear_tool.parse_args()
    yt_client = ytcli.create_client(server=cli_args.server, be_verbose=False)
    yt_clear_tool.run_clearing(
        path_to_clear=cli_args.path,
        yt_client=yt_client,
        access_time_limit=cli_args.access_time_limit or cli_args.access_timeout_limit,
        creation_time_limit=cli_args.creation_time_limit,
        modification_time_limit=cli_args.modification_time_limit,
        invert_access_time_limit=cli_args.invert,
        owners=cli_args.owners,
        substring=cli_args.substring,
        substring_blacklist=cli_args.without_substring,
        remove_really=cli_args.remove_really,
        node_type=cli_args.node_type,
        verbose=cli_args.verbose,
    )


if __name__ == "__main__":
    main()
