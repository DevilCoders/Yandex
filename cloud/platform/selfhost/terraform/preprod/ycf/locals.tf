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
    "ru-central1-a" = "bucpba0hulgrkgpd58qp"
    "ru-central1-b" = "bltueujt22oqg5fod2se"
    "ru-central1-c" = "fo27jfhs8sfn4u51ak2s"
  }

  conductor_group = "ycf"

  arbitrary_metadata = {
    osquery_tag = "ycloud-svc-serverless"
  }

  hc_interval = "2s"
}
