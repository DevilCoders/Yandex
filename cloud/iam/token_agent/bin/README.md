# Token agent initialization scripts

Used to initialize the token agent during the bootstrap process.
By default, `token-agent-init` creates one key per user service account and
reuses existing keys if possible. To create keys for group use `--group` flag.
To force create new keys use `--force` flag. To create one key per host use 
`--key-policy PER_HOST`, to create one key per user use `--key-policy PER_USER`.

## token-agent-init

Configuration file (default is `/etc/yc/token-agent/config.yaml`):

```yaml
tpmAgentEndpoint:
  host: /var/run/yc-tpm-agent/yc-tpm-agent.sock

rolesPath: /var/lib/yc/token-agent/roles
```
To initialize the token agent with a static token (should be used on the seed only):

```bash
./token-agent-init --config=/path/to/config.yaml --static-token <TOKEN> user1 user2[/tag] ... userN
```

The `tag` is optional qualifier to allow multiple tokens for one user.

To initialize the token agent:

1) invoke `token-agent-init` to generate keys

```bash
$ ./token-agent-init --config=/path/to/config.yaml user1[/tag]:sa1 user2:sa2 ... userN:saN

"tokenAgentKeys":
  "user1":
    "publicKey": "-----BEGIN PUBLIC KEY-----\nMII...AB\n-----END PUBLIC KEY-----"
    "serviceAccountId": "service_account1"
  "user2/tag":
    "publicKey": "-----BEGIN PUBLIC KEY-----\nMII...AB\n-----END PUBLIC KEY-----"
    "serviceAccountId": "service_account2"
  "userN":
    "publicKey": "-----BEGIN PUBLIC KEY-----\nMII...AB\n-----END PUBLIC KEY-----"
    "serviceAccountId": "service_accountN"
```

2) call GRPC method yandex.cloud.priv.iam.v1.KeyService/Create for each public key

```bash
$ grpcurl -H 'Authorization: Bearer ...' \
  -d '{"service_account_id": "service_account1", "public_key": "-----BEGIN PUBLIC KEY-----\nMIIB...QAB\n-----END PUBLIC KEY-----"}' \
  <iam-control-plane-endpoint>:4283 yandex.cloud.priv.iam.v1.KeyService/Create

{
  "key": {
    "id": "key_id1",
    "service_account_id": "service_account1",
    "created_at": "2020-10-28T14:56:47.023675Z",
    "key_algorithm": "RSA_2048",
    "public_key": "-----BEGIN PUBLIC KEY-----\nMIIB...QAB\n-----END PUBLIC KEY-----\n"
  }
}
```

OR

```bash
ycp iam key create -r - <<REQ
service_account_id: "service_account1"
public_key: "-----BEGIN PUBLIC KEY-----\nMIIB...QAB\n-----END PUBLIC KEY-----"
REQ
```

3) invoke `token-agent-init` with `--bind-keys` parameter:

```bash
$ cat token-service-response.yaml
"tokenAgentKeys":
  "user1":
    "keyId": "key_id1"
  "user2/tag":
    "keyId": "key_id2"
  "userN":
    "keyId": "key_idN"

$ ./token-agent-init --config=/path/to/config.yaml --bind-keys - < token-service-response.yaml
```
4) invoke `token-agent-init` for groups:
```bash
$ ./token-agent-init --group --config=/path/to/config.yaml group1[/tag]:sa1 group2:sa2 ... groupN:saN

"tokenAgentKeys":
  "group1":
    "publicKey": "-----BEGIN PUBLIC KEY-----\nMII...AB\n-----END PUBLIC KEY-----"
    "serviceAccountId": "service_account1"
  "group2/tag":
    "publicKey": "-----BEGIN PUBLIC KEY-----\nMII...AB\n-----END PUBLIC KEY-----"
    "serviceAccountId": "service_account2"
  groupN":
    "publicKey": "-----BEGIN PUBLIC KEY-----\nMII...AB\n-----END PUBLIC KEY-----"
    "serviceAccountId": "service_accountN"
```
### Debug

To enable verbose output use `-v`, `-vv` or `-vvv` flags.

### Usage

```bash
usage: token-agent-init [-h] [-b BIND_KEYS] [-c CONFIG]
                        [-k {PER_HOST,PER_SERVICE_ACCOUNT,PER_USER}]
                        [-s STATIC_TOKEN] [-v[v[v]]]
                        [role[/tag][:sa] [role[/tag][:sa] ...]]

positional arguments:
  role[/tag][:sa]       Linux user or group and optional service account and tag to initialize

optional arguments:
  -h, --help            show this help message and exit
  -g, --group           Apply changes to groups
  -b, --bind-keys <BIND_KEYS>
                        Bind public keys from yaml file; use "-" for stdin
  -c, --config <CONFIG>
                        Config file
  -f, --force           Never reuse exsisting TPM keys
  -k, --key-policy <PER_HOST|PER_SERVICE_ACCOUNT|PER_USER>
                        Key creation policy
  -s, --static-token <STATIC_TOKEN>
                        Init with static token from file; use "-" for stdin
  -v, --verbose         Verbose output
```
