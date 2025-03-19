## Access service dev client

Request Access Service to check `license check` permissions for specified token bearer.

### Build
```bash
$ ya make
```

### Check permission
```bash
$ ./dev r as.private-api.cloud-preprod.yandex.net:4286 -c <path/to/cert>  authorize  -t <iam-token>
```