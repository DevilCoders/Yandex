## Setup your pillar

More details - https://wiki.yandex-team.ru/passport/tvm2/tvm-daemon/#konfig


## You need your bb.y-t only?

It setup tvmdaemon with one destination - yateam blackbox, with alias `blackbox`.


### Public pillar

```yaml
data:
    tvmtool:
        port: 5001
        tvm_id: 42
```


### Private pillar

```yaml
data:
    tvmtool:
        token: 'XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX'
        config:
            secret: 'YYY'
```

`data:tvmtool:token` is a token for your application communication with TVM Daemon.
Should be a 32-character string.
*Don't copy-paste it* - generate it:

```bash
xxd -l 16 -p /dev/urandom
```

`data:tvmtool:config:secret` is your TVM secret


## More then one client, custom destinations


### Public pillar
```yaml
data:
    tvmtool:
        config:
            clients:
                mdb-event-producer:
                    tvm_id: 42
                        dsts:
                            logbroker:
                                dst_id: 2001059
                            blackbox:
                                dst_id: 1111
                mdb-search-producer:
                    tvm_id: 42
                        dsts:
                            logbroker:
                                dst_id: 2001059
                            blackbox:
                                dst_id: 2222
```

### Private pillar

```yaml
data:
    tvmtool:
        config:
            clients:
                mdb-event-producer:
                    secret: 'XXXX'
                mdb-search-producer:
                    secret: 'YYYY'
                    tvm_id: 42
                        dsts:
                            logbroker:
                                dst_id: 2001059
                            blackbox:
                                dst_id: 2222
```

