locals {
  s3_endpoint = "https://storage.cloud-preprod.yandex.net"
  s3_region   = "us-east-1"

  trace_collector_endpoint = "jaeger-agent.ycp.cloud-preprod.yandex.net"
  trace_collector_port     = "5775"

  zones = [
    "ru-central1-a",
    "ru-central1-b",
    "ru-central1-c",
  ]

  zones_short = {
    "ru-central1-a" = "vla"
    "ru-central1-b" = "sas"
    "ru-central1-c" = "myt"
  }

  subnets = {
    "ru-central1-a" = "bucoeaf6j09gu7s6l6t1"
    "ru-central1-b" = "blts5hts1gbn3hjee06t"
    "ru-central1-c" = "fo2rp85j9rdav26cjpc5"
  }

  conductor_group = "ycf"

  arbitrary_metadata = {
    osquery_tag = "ycloud-svc-mdbproxy"
  }

  hc_interval = "2s"
}
