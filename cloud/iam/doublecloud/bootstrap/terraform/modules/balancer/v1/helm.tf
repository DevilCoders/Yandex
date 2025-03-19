locals {
  helm_chart_name                 = "load-balancers"
  helm_chart_path                 = "../../../../../helm/datacloud-load-balancer"
  helm_chart_templates_path       = "${local.helm_chart_path}/templates"
  helm_chart_templates_files_hash = substr(sha256(join("", [for k in fileset(local.helm_chart_templates_path, "**") : filesha256("${local.helm_chart_templates_path}/${k}")])), 0, 63)
  helm_chart_values_files_hash    = substr(sha256(join("", [for k in fileset(local.helm_chart_path, "**values.yaml") : filesha256("${local.helm_chart_path}/${k}")])), 0, 63)
  env_path                        = "${var.env_name}/${var.cloud_provider}"

  iam_public_alb_helm_config      = {
    "serviceType"  = "ClusterIP"
    "targetGroups" = [
      {
        alias       = "oauth"
        targetType  = "ip"
        serviceName = local.service_ports_https.oauth.service_name
        portName    = local.service_ports_https.oauth.port_name
        port        = aws_lb_target_group.oauth.port
        arn         = aws_lb_target_group.oauth.arn
      },
      {
        alias       = "auth-ui"
        targetType  = "ip"
        serviceName = local.service_ports_others.auth_ui.service_name
        portName    = local.service_ports_others.auth_ui.port_name
        port        = aws_lb_target_group.auth_ui.port
        arn         = aws_lb_target_group.auth_ui.arn
      },
    ]
  }
}

// Write configuration values for helm to use
resource "local_file" "helm_values" {
  filename        = "${local.helm_chart_path}/${local.env_path}/terraform_values.yaml"
  file_permission = "0440"

  content = yamlencode({
    "loadBalancers" = {
      "nlb"            = module.iam_nlb.helm_lb_config
      "tls-nlb"        = module.iam_tls_nlb.helm_lb_config
      "public-alb"     = local.iam_public_alb_helm_config
      "operations-alb" = module.iam_operations_alb.helm_lb_config
    }
  })

  depends_on = [
    module.iam_nlb,
    module.iam_tls_nlb,
    aws_lb_target_group.oauth,
    aws_lb_target_group.auth_ui,
  ]
}

resource "helm_release" "load_balancers" {
  chart             = local.helm_chart_path
  name              = local.helm_chart_name
  cleanup_on_fail   = true
  lint              = true
  timeout           = 600
  wait_for_jobs     = true
  atomic            = true
  dependency_update = true

  values = [
    file("${local.helm_chart_path}/values.yaml"),
    file("${local.helm_chart_path}/${local.env_path}/values.yaml"),
    file("${local.helm_chart_path}/${local.env_path}/terraform_values.yaml"),
  ]

  // Following variables watch directories for changes and force 'helm upgrade' on changes
  set {
    name  = "helm_chart_templates_files_hash"
    value = local.helm_chart_templates_files_hash
  }

  set {
    name  = "helm_chart_values_files_hash"
    value = local.helm_chart_values_files_hash
  }

  depends_on = [
    local_file.helm_values,
  ]
}
