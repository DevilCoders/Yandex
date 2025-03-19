locals {
  chart_templates_path       = "${var.chart}/templates"
  chart_config_path          = "${var.chart}/${var.env_name}/${var.cloud_provider}"
  chart_templates_files_hash = substr(sha256(join("", [for k in fileset(local.chart_templates_path, "**") : filesha256("${local.chart_templates_path}/${k}")])), 0, 63)
  chart_values_hash          = substr(filesha256("${var.chart}/values.yaml"), 0, 63)
  chart_config_values_hash   = substr(filesha256("${local.chart_config_path}/values.yaml"), 0, 63)
}

data "sops_file" "secrets" {
  source_file = "${local.chart_config_path}/${var.name}/secrets.yaml"
}

resource "helm_release" "chart" {
  name             = var.name
  repository       = null
  chart            = var.chart
  version          = var.chart_version
  namespace        = var.namespace
  create_namespace = false
  cleanup_on_fail  = true
  wait_for_jobs    = true
  atomic           = true
  timeout          = var.timeout

  values = [
    file("${var.chart}/values.yaml"),
    file("${local.chart_config_path}/values.yaml"),
    file("${local.chart_config_path}/${var.name}/files/vars.yaml"),
    data.sops_file.secrets.raw,
  ]

  // Following variables watch directories for changes and force 'helm upgrade' on changes
  set {
    name  = "chart_templates_files_hash"
    value = local.chart_templates_files_hash
  }

  set {
    name = "chart_config_service_files_hash"
    value = substr(sha256(join("", [for k in fileset("${local.chart_config_path}/${var.name}", "**") : filesha256("${local.chart_config_path}/${var.name}/${k}")])), 0, 63)
  }

  set {
    name  = "chart_values_hash"
    value = local.chart_values_hash
  }

  set {
    name  = "chart_config_values_hash"
    value = local.chart_config_values_hash
  }

// TODO
//  dynamic "set_sensitive" {
//    for_each = var.set_sensitive_values
//    content {
//      name  = set_sensitive.value.name
//      value = set_sensitive.value.value
//    }
//  }
}
