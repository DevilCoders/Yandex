locals {
  iam_endpoint           = "iam.api.cloud.yandex.net"
  monitoring_url         = "https://monitoring.api.cloud.yandex.net/monitoring/v2/data/write"
  envoy_metrics_pull_url = "http://localhost:8002/stats/prometheus"
}

resource "kubectl_manifest" "contour_envoy_ua_configmap" {
  override_namespace = "default"

  yaml_body = templatefile("${path.module}/../../configs/contour-envoy-ua-configmap.tpl", {
    folder_id              = var.folder_id
    monitoring_url         = local.monitoring_url
    iam_endpoint           = local.iam_endpoint
    envoy_metrics_pull_url = local.envoy_metrics_pull_url
  })
  wait = true
}
