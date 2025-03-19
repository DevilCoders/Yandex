# Token agent daemon

## Usage

```bash
./yc-token-agent --log-level ERROR|WARNING|INFO|DEBUG --config /path/to/config.file \
  --access-log-output /path/to/accesslog.file --log-output /path/to/log.file
```

Default log level is `INFO`.
Default log output is stderr.
Default configuration file is `/etc/yc/token-agent/config.yaml`

## GRPC diagnostic

To enable GRPC diagnostic use environment variables:

```bash
GRPC_TRACE=all
GRPC_VERBOSITY=all
```

More details in [GRPC docs](https://github.com/grpc/grpc/blob/master/doc/environment_variables.md).
