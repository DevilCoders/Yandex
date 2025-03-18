from build.plugins.lib.nots.typescript import TsBundleWrapper


class TsError(RuntimeError):
    pass


class TsCompilationError(TsError):
    def __init__(self, code, stdout, stderr):
        self.code = code
        self.stdout = stdout
        self.stderr = stderr

        super(TsCompilationError, self).__init__("tsc exited with code {}:\n{}\n{}".format(code, stdout, stderr))


def bundle_webpack_parser(subparsers):
    cmd = subparsers.add_parser("bundle-webpack", help="run webpack")
    cmd.add_argument("--build-root", required=True)
    cmd.add_argument("--bindir", required=True)
    cmd.add_argument("--curdir", required=True)
    cmd.add_argument("--nodejs-bin", required=True)
    cmd.add_argument("--webpack-resource", required=True)
    cmd.add_argument("--webpack-script", required=True)
    cmd.add_argument("--webpack-config", required=False)
    cmd.add_argument("--ts-config", required=False)
    cmd.set_defaults(func=bundle_webpack_func)


def bundle_webpack_func(args):
    ts_bundle = TsBundleWrapper(
        build_root=args.build_root,
        build_path=args.bindir,
        sources_path=args.curdir,
        nodejs_bin_path=args.nodejs_bin,
        script_path=args.webpack_script,
        ts_config_path=args.ts_config,
        webpack_config_path=args.webpack_config,
        webpack_resource=args.webpack_resource,
    )

    ts_bundle.compile()
