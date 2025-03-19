module "iam_service_backend_groups" {
  source = "../modules/backend_group/v1"

  for_each = local.iam_service_settings

  is_http               = each.value.is_http
  name                  = format("%s-backend", each.value.name)
  backend_name          = lookup(each.value, "backend_name", null)
  description           = lookup(each.value, "description", null)
  backend_port          = each.value.backend_port
  healthcheck_port      = each.value.healthcheck_port
  http_healthcheck_path = each.value.http_healthcheck_path
  backend_weight        = lookup(each.value, "backend_weight", 100)

  folder_id       = local.iam_ya_prod_folder.id
  target_group_id = ycp_platform_alb_target_group.iam_ya_target_group.id
  environment     = "Yandex-Team (iam-ya)"
}
