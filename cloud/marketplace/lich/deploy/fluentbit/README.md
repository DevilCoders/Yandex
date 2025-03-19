### YC Fluent-bit container

FluentBit Docker container with YC PrivateAPI Logging plugin. YCP allows to write entries without
RPS quota. Container is self contained: all configs and required plugins binaries are packed into image.
Basic configuration done via environment variables:
  - YC_PLUGIN_NAME: Yandex plugin to stream logs via: yc-logger - use public api, ycp-logger - use private api, use ycp-logger in most cases as privet api doesn't have rate limiter.
  - YC_LOGGING_GROUP_DEFAULT: Default _marketplace_ log group, logs from all inputs written to this group.
  - YC_LOGGING_GROUP_ACCESS: Nginx access logs, could contain user sensitive information, stored as raw string for now, but could be parsed for structured logs support.
  - YC_LOGGING_GROUP_BACKENDS: Logs for private-api marketplace services, including license-check.
  - YC_ENDPOINT: YC Logging ingestion endpoint, note that private/public-api share the same host, but differs only in port value.
  - YC_CACERT: Container path to root certificates combined file, in most cases link to mounted host `/etc/ssl/certs/ca-certificates.crt` file.

#### Deployment via Makefile

See sample [Makefile](./Makefile) for deployment variants.
