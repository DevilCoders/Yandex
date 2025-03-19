## Run options

```
$ lcl request --in-file prod.requests.mongo.single.input -t $YC_IAM_TOKEN -a 'http://host:port/marketplace/v2/license/check/' --host 'mkt.private-api.cloud.yandex.net'
```
 - _--in-file_ - input request
 - _--token_ - iam token to make request with
 - _a_ - api endpoint: base url with api path
 - _host_ - (optional) host header to be send with request

### View help
```
$ lcl <command> --help
```

## Input format

File with json record to form up the request body, on per line:
```
{"cloud_id":"id1","product_ids":["prodID1","prodID2"]}
...
```

## Output format

First field is request body, then follows response http status code and response body, all field are tab separated:
```
{"cloud_id":"id1","product_ids":["prodID1","prodID2"]}   200   {}
...
```

