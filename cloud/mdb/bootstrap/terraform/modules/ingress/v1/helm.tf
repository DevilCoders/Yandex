locals {
  chart_name = "yc-alb-ingress-controller"
}

resource "null_resource" "download_chart" {
  triggers = {
    always_run = timestamp()
  }
  provisioner "local-exec" {
    command = <<-EOT
      export HELM_EXPERIMENTAL_OCI=1
      helm chart remove cr.yandex/crpsjg1coh47p81vh2lc/${local.chart_name}-chart:v0.0.4
      helm chart pull cr.yandex/crpsjg1coh47p81vh2lc/${local.chart_name}-chart:v0.0.4
      helm chart export cr.yandex/crpsjg1coh47p81vh2lc/${local.chart_name}-chart:v0.0.4 --destination ./install
    EOT
  }
}

module "ingress_release" {
  source = "../../helm/v1"
  providers = {
    helm = helm
  }
  timeout       = 240
  chart         = "./install/${local.chart_name}"
  chart_version = "0.0.2"
  name          = "yc-alb-ingress-controller"
  namespace     = kubernetes_namespace.namespace.metadata[0].name
  values = [
    "subnetIds: ${join(",", var.subnet_ids)}"
  ]
  set_values = [
    {
      name  = "folderId",
      value = var.folder_id
    },
    {
      name  = "clusterId",
      value = var.cluster_id
    }
  ]
  depends_on = [null_resource.download_chart]
}
