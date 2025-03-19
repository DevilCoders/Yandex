// export YC_TOKEN=$(yc --profile=prod iam create-token)
// terraform init -backend-config="secret_key=$(ya vault get version ver-01e9d91yf5qmcq891em025rwgr -o s3_secret_access_key 2>/dev/null)"
//
locals {
  team_registries = toset(flatten([keys(var.team_registries_push_permissions), keys(var.team_registries_pull_permissions)]))
}
resource "yandex_container_registry" this {
  for_each = local.team_registries
  name     = each.value
  labels   = {}
}

resource "yandex_container_registry_iam_binding" this_pull {
  for_each    = var.team_registries_pull_permissions
  registry_id = yandex_container_registry.this[each.key].id
  role        = "container-registry.images.puller"
  members     = each.value
}

resource "yandex_container_registry_iam_binding" this_push {
  for_each    = var.team_registries_push_permissions
  registry_id = yandex_container_registry.this[each.key].id
  role        = "container-registry.images.pusher"
  members     = each.value
}

resource "yandex_container_registry_iam_binding" this_pull_custom {
  for_each    = var.custom_registries_pull_permissions
  registry_id = each.key
  role        = "container-registry.images.puller"
  members     = each.value
}
