
## Collect error codes

### For PgBouncer
```bash
while true; do
    ps axu |grep checkpoint | grep -v grep | awk '{print $2}' | xargs kill -9;
    sleep 1;
    service pgbouncer status | grep PID | awk '{print $3}' | xargs kill -9;
sleep 1.5;
done
```

### For Odyssey
```bash
while true; do
    ps axu |grep checkpoint | grep -v grep | awk '{print $2}' | xargs kill -9;
    sleep 1;
    service odyssey status  | grep 'Main PID' | awk '{print $3}' | xargs kill -9;
done
```