
## Salt-formula: push-client

**State apply:**
```
salt-call state.sls common.push-client
```

**List of available pillars:**

```yaml
push_client:
  enabled: True
  # basic configs parameters
  defaults:
    ident: yourident
    host: localhost
    dc: my
    tvm:
       enabled: True
       client_id: 1234567
       server_id: 2001059
       secret_file: /var/lib/yc/push-client/tvm.secret

  instances:
    # override basic configs parameters for concrete instance of push-client
    myinstance:
      ident: myinstance-ident
      host: myinstance-host
      dc: my
      tvm: {...}
      files:
        - name: /etc/log/abc.log
          ident: concrete ident for file (optional)
          log_type: yourlogtype
          send_delay: 100
```


### Parameters description:

Any topic name of logbroker (https://lb.yandex-team.ru/main/topic ) consists of the following parts:

`rt3.<dc>--<ident>--<log_type>`

so, push-client configuration require all these params to send logs into specific destination.  

*  `ident` - producer identifier (subject of ACL)
*  `host` - logbroker entrypoint url. default is load-balancer - logbroker.yandex.net
*  `dc` - datacenter affinity, by defaul - 'current', on fails chooses any  available.
*  `tvm` - tvm configs (optional)
*  `files` - list of files to send. `log_type` is required

### Enviroment based idents:

There are several pre-defined idents that are specified by default at enviroments pillars :

- **ident**: yc-df
	- **file**: `pillar/common/df.sls`
- **ident**: yc-df-pre
	- **file**: `pillar/common/pre-df.sls`
- **ident**: yc
	- **file**: `pillar/common/prod.sls`
- **ident**: yc-pre
	- **file**: `pillar/common/pre-prod.sls`

So if you are going to supply logs/metrics to the **logbroker**, it's **advisable** to reuse these idents to create new topics with names:`<pre-defined ident>--<your log_type>`

### Auth/Authz (TVM):

There are two pre-defined  tvm apps at Yandex.Cloud ABC service:

- **tvm testing:**
	- **client_id**:  `2001289`
	- **secdist path:** `/repo/projects/yc-testing-iam/logbroker/tvm`
- **tvm prod:**
	- **client_id**: `2001287`
	- **secdist path:** `/repo/projects/yc-prod-iam/logbroker/tvm`

**Warning:** In most cases, you  should re-use these client ids, because of push-client  can only use one secret at a time.

In order to delivery secrets to specific deployment role, follow guides at:

- https://github.yandex-team.ru/YandexCloud/bootstrap-templates



### Basic configuration steps:

1. Include  `common.push-client` into your `salt-formula/roles/<role>.sls` file
2. Specify a list of sending files (logs) in `push_client` pillar  at `salt-formula/pillar/roles/<role>.sls`. See examples at `compute.sls`

After these steps, your files will be supplied using one of the pre-defined idents and one of the tvm client ids, depending on enviroment.

### Links:

- https://wiki.yandex-team.ru/logbroker/docs/push-client/config/
