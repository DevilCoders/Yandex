# Set alerts
put [oauth token](https://docs.yandex-team.ru/solomon/api-ref/authentication#oauth) for solomon in `.nbs-tools/secrets.json`
```
$ grep solomon .nbs-tools/secrets.json
   "solomon_oauth_token": "XXXXXXXXXX",
```
check alerts diff
```
./set_alerts -c configs/blockstore.yaml -f alerts/blockstore --diff
./set_alerts -c configs/disk_agent.yaml -f alerts/disk_agent --diff
./set_alerts -c configs/disk_manager.yaml -f alerts/disk_manager --diff
```
apply alerts
```
./set_alerts -c configs/blockstore.yaml -f alerts/blockstore --apply
./set_alerts -c configs/disk_agent.yaml -f alerts/disk_agent --apply
./set_alerts -c configs/disk_manager.yaml -f alerts/disk_manager --apply
```
diff only for a specific cluster
```
./set_alerts -c configs/disk_manager.yaml -f alerts/disk_manager --cluster testing --diff
```

### update all project configuration
- NBS
```
./set_alerts -c configs/blockstore.yaml --alerts-path alerts/blockstore --channels-path channels/blockstore.yaml --clusters-path clusters/blockstore.yaml --services-path services/blockstore --shards-path shards/blockstore --dashboards-path dashboards/blockstore --diff

IAM_TOKEN=$(ycp --profile=israel iam create-token) ./set_alerts -c configs/blockstore_israel.yaml --alerts-path alerts/blockstore --channels-path channels/blockstore.yaml --clusters-path clusters/blockstore.yaml --services-path services/blockstore --shards-path shards/blockstore --dashboards-path dashboards/blockstore --diff
```
- Disk Agent
```
./set_alerts -c configs/disk_agent.yaml -f alerts/disk_agent --apply
```
- Disk Manager
```
./set_alerts -c configs/disk_manager.yaml --alerts-path alerts/disk_manager --channels-path channels/disk_manager.yaml --clusters-path clusters/disk_manager.yaml --services-path services/disk_manager --shards-path shards/disk_manager --dashboards-path dashboards/disk_manager --apply

IAM_TOKEN=$(ycp --profile=israel iam create-token) ./set_alerts -c configs/disk_manager_israel.yaml --alerts-path alerts/disk_manager --channels-path channels/disk_manager.yaml --clusters-path clusters/disk_manager.yaml --services-path services/disk_manager --shards-path shards/disk_manager --dashboards-path dashboards/disk_manager --apply
```
