## mdb-controlplane-telegraf

It includes Telegraf and its config with configured Prometheus `output`.
That component doesn't require additional configuration.

Use the same `data:solomon:*` as mdb-metrics but:
- Sends system (io, disk, net ...) metrics to `system_tgf` service
- Sends Prometheus metrics to `mdb_tgf` service.
