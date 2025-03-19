# Yandex.Cloud Support Metrics Collector

Collector of various metrics of requests to the Yandex Cloud support service.

- Issues with metadata  
- Support response with metadata  
- SLA  


## Get started

### Config setup

**Get startrek internal token** [here](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=a7597fe0fbbf4cd896e64f795d532ad2)

Default path: `~/.yc-support/config.yaml`

You can also use your own path to the config file:
`collector --config /path/to/config.yaml`

**Config example:**
```yaml
startrek:
  token: AQAAqweqwezxczxcqweqweXczczsw
  filter: 'Queue: CLOUDSUPPORT and Updated: >= now() - 1d'

# for uploading in run-forever mode only
database:
  host: host.example.com
  port: 3306
  user: collector
  passwd: your-password
  db_name: cloudsupport
  ca_path: /path/to/root.crt

logs:
  path: /home/username/
  level: info
  max_size: 1048576
  backup_count: 2
```

**The maximum file size must be specified in bytes**  
**Log levels:** debug, info, warning, error.

### Usage

If the filter is not specified in the config file, the default queue filter is used: `Queue: CLOUDSUPPORT and Updated: >= now() - 1d`  
  
You can change the filter by specifying it in the config file,
or using the `--filter` argument. See the examples below.  

The `--filter` argument replaces the value from the config file.

**Examples**
  
- Print external comments from issue as table:  
```bash
collector -i CLOUDSUPPORT-123
```
  
- Export external comments to CSV:  
```bash
collector --export --filter "Queue: CLOUDSUPPORT and Updated: >= now() - 2h"
```
  
- Infinite collection of metrics with an interval:  
```bash
collector -r --interval 600 --filter "Queue: CLOUDSUPPORT and Updated: >= now() - 6h" --config ~/.config/config.yaml --write-logs
```
  

**Help message**
  
```
usage: collector [-h] [-v] [-i key] [-e] [--clean] [-r] [--write-logs]
                 [--config file] [--interval int] [--filter str]

Support issue metrics collector.

optional arguments:
  -h, --help           show this help message and exit
  -v, --version        show program's version number and exit
  -i key, --issue key  print issue comments. key must be CLOUDSUPPORT-123
  -e, --export         export comments to CSV file
  --clean              clean all non support comments from DB
  -r, --run-forever    run forever collector with upload to DB
  -l, --write-logs     write logs to /var/log/ or path from config.
  --config file        path to config file
  --interval int       interval in secs for `run-forever`
  --filter str         queue filter, like a "Queue: CLOUDSUPPORT and Updated: >= now() - 1d"
```