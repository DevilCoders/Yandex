```bash
./add_agents \
    --hosts new-agents.txt \
    --device-count XXX \
    --ticket NBSOPS-XXX \
    --service-config ./config.yaml \
    --cluster-name prod \
    --zone-name sas \
    --patcher-dir ../../cms/patcher \
    --verbose

```
После выполнения этой команды нужно закоммитить disk_registry_config.txt и patch.json
