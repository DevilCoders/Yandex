# mongodb_cache
Django-based cache, using mongodb replicaset as a storage

This is useful in distributed systems, which need cache, that is the same on all machines and somehow memcached is not a choice for them. Memcached can be a problem, cause it wipes out entities when memory limit is hit. Sometimes you want to be sure the cache value is always there where you put it, so mongodb is useful. 

mongodb_cache takes benefit of MongoDB's TTL collections. 

## Settings 

Example configuration:
```
    'given_tokens': {  # name of cache
        'BACKEND': 'mongodb_cache.MongoDBCache',
        'LOCATION': 'mongodb://host1:27017,host2:27017/mydatabase',
        'collection': 'given_tokens',
        'OPTIONS': {
            'replica_set': 'wiki',
        }
    },
```

```LOCATION```: should be a mongodb connection string, like 'mongodb://host1:27017,host2:27017/mydatabase' in common installations.
```collection``` - name of collection in mongodb. Required parameter.
```TIMEOUT```: timeout in seconds. Not required, has a default.
``replica_set```: name of replicaset, required.


### OPTIONS

All options are non-required, have some reasonable defaults.

```'WRITE_CONCERN': 0```, defaults to 1 (safe but more slow to write values). This is passed to mongodb-client. 0 allowes unsafe quick writes, 1 means at least 1 machine from mongodb replicaset will actually write the value.

```'VALUES_ARE_JSON_SERIALIZEABLE': False```, defaults to True (unsafe in common sense, but quickly saves space in mongodb). Set it to False and all your values will be pickled and base64 encoded before writes into mongodb. This will ensure that any of your pickable object will be written to mongodb without errors. Leave the default of you store only json-serailizable values (simple structures like dicts, lists, strings, numbers etc).

```'STRATEGY': 'PRIMARY'```, defaults to 'NEAREST'. This is passed to mongodb client. NEAREST allows quicker reads comparing to PRIMARY. But PRIMARY allows consistent reads. PRIMARY is usefule if you store authentification tokens for example. You should always authenticate user if has been authentificated once until the token is expired.


