# Test your LUA scripts in docker container

* start container: `make up`
* stop container: `make down`

> NOTE: before starting the container for the first time, make sure you cleared the
> 'values.conf' from J2 template magic

The nginx in docker container will be available on `localhost:8888`
The actual nginx configuration is located at `conf.d/` and `nginx.conf`.
Changes in these files require container rebuild to take effect.

The S3 lib lua files are bind-mounted into the docker-container, the changes in these files
do not require container rebuild.

Nginx logs directory is also bind-mounted, so you can see logs on your local host without
need to enter the container

You also have data directory for bidirectional files exchange between container and host.
It is bind-mounted as /data inside the container.

Example of curl request to container:
```shell
curl -v --header 'Host: s3.mds.yandex.net' 'http://localhost:8080/'
```


## Settings and tips you might find useful:

### 'Unexpected { in values.conf'
Just comment or remove all leading J2 template magic strings starting with `{#`, `{%`, and `{{`.
They confuse nginx config parser.

### Public and private buckets access settings sync
set private-api.json to this to make external buckets sync code work in container:
```shell
{
  "endpoint": "http://127.0.0.1:8888",
  "host_header": "s3-idm.mdst.yandex.net"
}
```

Now you can see non-empty list on
`curl --silent --header 'Host: s3-private.mds.yandex.net' 'http://localhost:8080/tests/external-access/get-allowed-regexes'` call

Have fun!
