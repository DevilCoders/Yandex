locals {
  s3_endpoint = "https://storage.yandexcloud.net"
  s3_region   = "us-east-1"

  trace_collector_endpoint = "jaeger-agent.ycp.cloud.yandex.net"
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
    "ru-central1-a" = "e9b9e47n23i7a9a6iha7"
    "ru-central1-b" = "e2lt4ehf8hf49v67ubot"
    "ru-central1-c" = "b0c7crr1buiddqjmuhn7"
  }

  conductor_group = "ycf"

  arbitrary_metadata = {
    osquery_tag = "ycloud-svc-serverless"
  }

  hc_interval = "2s"
}
