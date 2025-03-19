# ROOT bindings
resource "ycp_resource_manager_root_iam_binding" "mdb_internal_api_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.ydpagent", # https://st.yandex-team.ru/MDB-9655
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.mdb_internal_api_sa.id}"
  ]
}

# https://st.yandex-team.ru/ORION-600
resource "ycp_resource_manager_root_iam_binding" "mdb_worker_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.mdbagent",
    "internal.ydpagent",
    "dcnetwork.editor", # https://st.yandex-team.ru/ORION-523
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.mdb_worker_sa.id}"
  ]
}

# https://st.yandex-team.ru/ORION-953
resource "ycp_resource_manager_root_iam_binding" "mdb_reaper_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "mdb.editor",
    "resource-manager.viewer",
  ])

  role    = each.key
  members = [
    "serviceAccount:${ycp_iam_service_account.mdb_reaper_sa.id}"
  ]
}

// TODO remove if after CLOUD-78785 roles will be released.
// http://bb.yandex-team.ru/projects/cloud/repos/cloud-go/browse/private-api/yandex/cloud/priv/mdb/v1/roles.yaml?at=4a651fcd5aad29df56b27cf628042a05226277f9#237
resource "ycp_resource_manager_root_iam_binding" "mdb_internal_api_sa_root_rmStageViewer" {
  role    = "internal.resource-manager.clouds.stageViewer"
  members = [
    "serviceAccount:${ycp_iam_service_account.mdb_internal_api_sa.id}"
  ]
}


# CLOUD bindings
resource "ycp_resource_manager_cloud_iam_member" "mdb_junk_cloud_mdb_internal_api_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "admin",
    "internal.computeadmin",
  ])

  cloud_id = ycp_resource_manager_cloud.mdb_junk_cloud.id
  role     = each.key
  member   = "serviceAccount:${ycp_iam_service_account.mdb_internal_api_sa.id}"
}

resource "ycp_resource_manager_cloud_iam_member" "mdb_junk_cloud_mdb_admin_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "admin",
  ])

  cloud_id = ycp_resource_manager_cloud.mdb_junk_cloud.id
  role     = each.key
  member   = "serviceAccount:${ycp_iam_service_account.mdb_admin_sa.id}"
}

# https://st.yandex-team.ru/ORION-852
resource "ycp_resource_manager_cloud_iam_member" "billing_service_cloud_mdb_billing_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.ydb.connect",
  ])

  cloud_id = ycp_resource_manager_cloud.billing_service_cloud.id # NB! BILLING service-cloud
  role     = each.key
  member   = "serviceAccount:${ycp_iam_service_account.mdb_billing_sa.id}"
}

# dataproc-e2e
resource "ycp_resource_manager_cloud_iam_member" "mdb_junk_cloud_mdb_e2e_tests_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "internal.computeadmin",
    "mdb.admin",
    "dcnetwork.editor",
  ])

  cloud_id = ycp_resource_manager_cloud.mdb_junk_cloud.id
  role     = each.key
  member   = "serviceAccount:${ycp_iam_service_account.mdb_e2e_tests_sa.id}"
}

# https://st.yandex-team.ru/MDB-10922
resource "ycp_resource_manager_cloud_iam_member" "mdb_clusters_cloud_mdb_worker_sa" {
  lifecycle {
    prevent_destroy = true
  }

  for_each = toset([
    "resource-manager.clouds.owner",
  ])

  cloud_id = ycp_resource_manager_cloud.mdb_clusters_cloud.id
  role     = each.key
  member   = "serviceAccount:${ycp_iam_service_account.mdb_worker_sa.id}"
}
