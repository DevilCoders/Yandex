# A Simple OK Cmd App

## Make

```
cd $ARCADIA_ROOT/library/python/ok_client/examples/ok_cmd
ya make
```

## Run

### Help

```
./ok_cmd --help
```

### Create an Approvement (Startrek)

```
./ok_cmd create <TICKET_KEY> <approver_1> [<approver_2> ...] [--text="<text>"] [--author=<author>]
```

### Get Info

```
./ok_cmd info <UUID>
```
