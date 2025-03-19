# ROOT bindings
resource "ycp_resource_manager_root_iam_binding" "iam_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "iam.onCallAdmin",
  ])
  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_service_account.id}"
  ]
}


resource "ycp_resource_manager_root_iam_binding" "iam_sync_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "iam.admin",
    "internal.billing.admin",
    "internal.iam.crossCloudBindings",
    "internal.iam.resourceTypes.admin",
    "internal.iam.sync",
    "internal.resource-manager.cloudsCreator",
    "internal.resource-manager.sync",
    "organization-manager.admin",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_sync_sa.id}"
  ]
}


resource "ycp_resource_manager_root_iam_binding" "iam_control_plane_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.iam.agent",
    "viewer",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_control_plane_sa.id}"
  ]
}


resource "ycp_resource_manager_root_iam_binding" "iam_openid_server_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.saas.agent",
    "internal.sessionService",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_openid_server_sa.id}"
  ]
}


resource "ycp_resource_manager_root_iam_binding" "iam_rm_control_plane_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.resource-manager.agent",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_rm_control_plane_sa.id}"
  ]
}


resource "ycp_resource_manager_root_iam_binding" "identity-sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.identityagent",
  ])
  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_common_sa.id}"
  ]
}


# GIZMO bindings
resource "ycp_resource_manager_gizmo_iam_binding" "iam_sync_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.iam.metaModelViewer",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_sync_sa.id}"
  ]
}

resource "ycp_resource_manager_gizmo_iam_binding" "iam_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.iam.metaModelEditor",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_service_account.id}"
  ]
}

resource "ycp_resource_manager_gizmo_iam_binding" "iam_org_service_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.organization-manager.agent",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.iam_org_service_sa.id}"
  ]
}


# RESOURCE bindings
resource "ycp_iam_resource_type_iam_binding" "organization_internal_iam_accessBindings_admin" {
  lifecycle {
    prevent_destroy = true
  }

  resource_type = "organization-manager.organization"
  role          = "internal.iam.accessBindings.admin"
  members       = [
    "serviceAccount:${ycp_iam_service_account.iam_openid_server_sa.id}",
  ]
}

resource "ycp_iam_resource_type_iam_binding" "organization_iam_admin" {
  lifecycle {
    prevent_destroy = true
  }

  resource_type = "organization-manager.organization"
  role          = "iam.admin"
  members       = [
    "serviceAccount:${ycp_iam_service_account.iam_org_service_sa.id}",
    "serviceAccount:${ycp_iam_service_account.iam_rm_control_plane_sa.id}", # CLOUD-56341 migrate cloud to organization
  ]
}

resource "ycp_iam_resource_type_iam_binding" "organization_internal_iam_listResourceTypeMemberships" {
  lifecycle {
    prevent_destroy = true
  }

  resource_type = "organization-manager.organization"
  role          = "internal.iam.listResourceTypeMemberships"
  members       = [
    "serviceAccount:${ycp_iam_service_account.iam_org_service_sa.id}",
    "serviceAccount:${ycp_iam_service_account.iam_rm_control_plane_sa.id}", # CLOUD-76343 list organization clouds by memberships
  ]
}

resource "ycp_iam_resource_type_iam_binding" "cloud_internal_iam_accessBindings_admin" {
  lifecycle {
    prevent_destroy = true
  }

  resource_type = "resource-manager.cloud"
  role          = "internal.iam.accessBindings.admin"
  members       = [
    "serviceAccount:${ycp_iam_service_account.iam_rm_control_plane_sa.id}",
  ]
}

resource "ycp_iam_resource_type_iam_binding" "folder_internal_iam_accessBindings_admin" {
  lifecycle {
    prevent_destroy = true
  }

  resource_type = "resource-manager.folder"
  role          = "internal.iam.accessBindings.admin"
  members       = [
    "serviceAccount:${ycp_iam_service_account.iam_rm_control_plane_sa.id}",
  ]
}


# FOLDER bindings
resource "ycp_resource_manager_folder_iam_member" "iam_openid_server_sa_folder_perms" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id = ycp_resource_manager_folder.iam_service_folder.id
  member    = format("serviceAccount:%s", ycp_iam_service_account.iam_openid_server_sa.id)
  role      = "kms.keys.encrypterDecrypter"
}
