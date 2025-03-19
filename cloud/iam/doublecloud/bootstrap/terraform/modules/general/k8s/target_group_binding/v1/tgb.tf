resource "kubectl_manifest" "tgb" {
  override_namespace = var.namespace
  wait               = true

  yaml_body = templatefile("${path.module}/tgb-template.yaml", {
    name             = var.name == null ? var.service : var.name
    service          = var.service
    port             = var.port
    target_group_arn = var.target_group_arn
  })
}
