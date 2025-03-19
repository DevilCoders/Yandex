# Token agent

## Configuration

Default configuration file is `/etc/yc/token-agent/config.yaml`

```yaml
keyPath: /var/lib/yc/token-agent/keys
cachePath: /var/cache/yc/token-agent/tokens
rolesPath: /etc/yc/token-agent/roles.d

cacheUpdateInterval: 10m
jwtAudience: https://iam.api.cloud.yandex.net/iam/v1/tokens
jwtLifetime: 1h

tokenServiceEndpoint:
  host: localhost
  port: 12345
  useTls: false
  timeout: 5s

tpmAgentEndpoint:
  host: /var/run/yc-tpm-agent/yc-tpm-agent.sock

listenUnixSocket:
  path: /path/to/grpc/server/socket.file
  backlog: 16
  mode: 0666
  group: daemon

httpListenUnixSocket:
  path: /path/to/http/server/socket.file
  backlog: 16
  mode: 0666
  group: daemon
```

Valid suffixes for intervals:

- `w` — Week
- `d` — Day
- `h` — Hour
- `m` — Minute
- `s` — Second

All roles should be in the directory specified by the parameter `rolesPath`
(default is `/etc/yc/token-agent/roles.d`). One role per user. To bind one
user to multiple service accounts use tag parameter (see below):

```
roles.d
 |-- user1.yaml
 |-- user2.yaml
 \-- user3
      |-- tag1.yaml
      \-- tag2.yaml
```

Each of them should be in the form

```yaml
token: "t1.9f7L7..."
expiresAt: 2019-01-22
```

or

```yaml
serviceAccountId: "fadedba0bab"
keyId: "deafdadb0b"
keyHandle: 783368690
```

If the `token` parameter is specified, its value will be returned to
all GetToken requests for this role. Parameter `expiresAt` is optional:
if omitted, the token will never expire.

Otherwise, the `keyId` and `serviceAccountId` parameters should be
specified to acquire a new IAM token from the `identity` service.

## To force token update

```bash
$ pkill -hup yc-token-agent
```

## Build instructions

```bash
ya package --debian iam/token_agent/package/pkg.json
```
To start the service, rename /etc/yc/token-agent/config.yaml.template:
```bash
sudo mv /etc/yc/token-agent/config.yaml.template /etc/yc/token-agent/config.yaml
```
and create /etc/yc/token-agent/roles.d/root.yaml:
```yaml
token: ANY_TOKEN
```
## Diagnose

To obtain the token:

```bash
sudo -Hu <user_name> grpcurl --plaintext --unix /var/run/yc/token-agent/socket yandex.cloud.priv.iam.v1.TokenAgent/GetToken
{
  "iam_token": "t1.9f7L7...",
  "expires_at": "2020-04-21T21:04:09.699317Z"
}
```

To obtain the token with `tag`:

```bash
sudo -Hu <user_name> grpcurl --plaintext -d '{"tag": "mysvc"}' --unix /var/run/yc/token-agent/socket yandex.cloud.priv.iam.v1.TokenAgent/GetToken
{
  "iam_token": "t1.9f7L7...",
  "expires_at": "2020-04-21T21:04:09.699317Z"
}
```

If the user is not allowed, the output should be

```
ERROR:
  Code: PermissionDenied
  Message: User 'wronguser' access denied
```

To obtain the token from HTTP server:

```bash
sudo -Hu <user_name> curl --unix-socket /var/run/yc/token-agent/http_socket http://localhost/tokenAgent/v1/token
{"iam_token":"t1.9f7L7...","expires_at":"2020-04-21T21:04:09.699317Z"}
```

If the user is not allowed, the output should be

```
<html><head><title>Forbidden</title></head><body><h1>403 Forbidden</h1><p>User 'wronguser' access denied</p></body></html>
```

Debug logs are in `/var/log/yc/token-agent.log`
