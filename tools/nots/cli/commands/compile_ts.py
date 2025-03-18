from build.plugins.lib.nots.typescript import TscWrapper


def compile_ts_parser(subparsers):
    cmd = subparsers.add_parser("compile-ts", help="compile ts")
    cmd.add_argument("--build-root", required=True)
    cmd.add_argument("--bindir", required=True)
    cmd.add_argument("--curdir", required=True)
    cmd.add_argument("--nodejs-bin", required=True)
    cmd.add_argument("--tsc-script", required=True)
    cmd.add_argument("--config", required=False)
    cmd.set_defaults(func=compile_ts_func)


def compile_ts_func(args):
    tsc = TscWrapper(
        build_root=args.build_root,
        build_path=args.bindir,
        sources_path=args.curdir,
        nodejs_bin_path=args.nodejs_bin,
        script_path=args.tsc_script,
        config_path=args.config,
    )

    tsc.compile()
