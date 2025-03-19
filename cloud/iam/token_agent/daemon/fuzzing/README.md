# Token Agent fuzzing

By default, the fuzzing is performed only for the parameter `tag`.
To enable USER ID fuzzing the program should be running by the root user.
Additionally, there should be two programs in the PATH: `sudo` and `grpcurl`.
Note that the `sudo` command will be executed with `--non-interactive` parameter.

Like this:

```bash
/bin/sudo --non-interactive --user #42 \
  /usr/bin/grpcurl --plaintext -d '{"tag": "foo"}' \
  --unix /path/to/socket yandex.cloud.priv.iam.v1.TokenAgent/GetToken
```

Or, one program named `test-helper` in the current directory.
To specify the location of `test-helper`, please set the variable `FUZZING_HELPER`:

```bash
export FUZZING_HELPER="$(readlink test-helper/test-helper)"
```

This helper program will be executed as follows for the same result:

```bash
 /path/to/test-helper /path/to/socket -u 42 -t foo
```
