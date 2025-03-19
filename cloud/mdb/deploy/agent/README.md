# Deploy agent for MDB VMs
https://wiki.yandex-team.ru/mdb/internal/teams/core/deployv3

## Local development
Build agent and it's cli
```bash
~ $ cd $ARCADIA_SOURCE_ROOT/cloud/mdb/deploy/agent/cmd/
~/C/g/s/a/c/m/d/a/cmd $ ya make
...
```
Launch agent
```bash
~ $ cd $ARCADIA_SOURCE_ROOT/cloud/mdb/deploy/agent/cmd/mdb-deploy-agent/
~/C/g/s/a/c/m/d/a/c/mdb-deploy-agent $ ./mdb-deploy-agent
{"level":"DEBUG","ts":"2022-06-22T11:38:31.455+0300","caller":"app/app.go:119","msg":"using application config","config":{"app":{"logging":{"level":"debug","file":""},"instrumentation":{"Addr":"[::1]:6060","ShutdownTimeout":1000000000,"Logging":{"LogRequestBody":false,"LogResponseBody":false}},"sentry":{"DSN":"xxx","Environment":""},"tracing":{"service_name":"","disabled":true,"local_agent_addr":"localhost:6831","queue_size":10000,"sampler":{"type":"const","param":1}},"solomon":{"project":"","service":"","cluster":"","url":""},"prometheus":{"url":"","namespace":""},"service_account":{"id":"","key_id":"xxx","private_key":"xxx"},"environment":{"services":{"iam":{"v1":{"token_service":{"endpoint":""}}}}}},"grpc":{"Addr":"unix:///tmp/mda.sock","ShutdownTimeout":1000000000,"Logging":{"LogRequestBody":false,"LogResponseBody":false}},"agent":{"max_history":500,"history_keep_age":"5m0s","salt":{"binary":"/bin/echo","args":[]},"shutdown_timeout":"1h0m0s"},"expose_error_debug":true,"retry":{"MaxRetries":2,"InitialInterval":0,"MaxInterval":0,"MaxElapsedTime":0},"s3":{"client":{"host":"s3.mds.yandex.net","region":"RU","access_key":"xxx","secret_key":"xxx","anonymous":true,"use_yc_metadata_token":false,"role":"","force_path_style":false,"transport":{"tls":{"CAFile":"","Insecure":false},"logging":{"LogRequestBody":false,"LogResponseBody":false}}},"bucket":"mdb-salt-images-test"},"call":{"stop_timeout":"1m0s"},"srv":{"srv_path":"/tmp/mdb-deploy-agent/srv","images_dir":"/tmp/mdb-deploy-agent/srv-images","tar_command":"tar"}}}
{"level":"INFO","ts":"2022-06-22T11:38:31.456+0300","caller":"httputil/http.go:38","msg":"serving http","addr":"[::1]:6060"}
{"level":"INFO","ts":"2022-06-22T11:38:31.463+0300","caller":"grpcutil/serve.go:99","msg":"serving gRPC","addr":"unix:///tmp/mda.sock"}
```
Test it with cli
```bash
~ $ cd $ARCADIA_SOURCE_ROOT/cloud/mdb/deploy/agent/cmd/mdb-deploy
~/C/g/s/a/c/m/d/a/c/mdb-deploy $ ./mdb-deploy -a 'unix:///tmp/mda.sock' run state.highstate
* resolving latest srv version, cause command doesn't specify it explicitly and there are no running commands
* updating srv '1655885400-r9618425' -> '1655886720-r9618425'
```