## Add a few settings

- Add something like this into `PostgresqlConfig` message in .common file
```
   string foo = -1;
   string bar = -1;
```

- Call `generate.sh` from this directory

## Add a new version

- Copy messages from previous version in .common files
- Add message names to `generate.sh`
- Call `generate.sh`

## Remove an old version

- remove all relative to this version messages from .common file
- remove all relative message-, file- names from `generate.sh`

