# -*- coding: utf-8 -*-
import pytlib.client_yt as client_yt


def create_yt_config_from_cli_args(cli_args, tmpfs=False):
    yt_config = client_yt.create_config(
        server=cli_args.server,
        verbose_operations=cli_args.verbose,
        quiet=cli_args.quiet,
        filter_so=cli_args.filter_so,
        mount_sandbox_in_tmpfs=tmpfs,
        ping_failed_mode=cli_args.ping_failed_mode,
    )
    return yt_config
