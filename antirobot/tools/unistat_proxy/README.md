# unistat_proxy

HTTP-server to simplify accessing a single unistat signal's value of a running service.

## Usage
Suppose you have a service that returns unistat metrics by `GET http://localhost:8899/admin?action=unistats`
Then you should run
```
unistat_proxy --http-port <any free port> --unistat-port 8899 --unistat-location "/admin?action=unistats"
```
After unistat_proxy is started, you can access your signals by `GET http://localhost:<port you specified>/?metric=my_lovely_signal`
