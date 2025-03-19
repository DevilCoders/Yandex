locals {
  iam_service_account_ids = [
    ycp_iam_service_account.iam_common_sa.id,
    ycp_iam_service_account.iam_access_service_sa.id,
    ycp_iam_service_account.iam_control_plane_sa.id,
    ycp_iam_service_account.iam_openid_server_sa.id,
    ycp_iam_service_account.iam_org_service_sa.id,
    ycp_iam_service_account.iam_reaper_sa.id,
    ycp_iam_service_account.iam_rm_control_plane_sa.id,
    ycp_iam_service_account.iam_token_service_sa.id,
  ]
}

resource "ycp_resource_manager_cloud" "iam_service_cloud" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id        = var.iam_project_id
  name            = "iam-service"
  organization_id = "yc.organization-manager.yandex"
}

resource "ycp_resource_manager_folder" "iam_service_folder" {
  lifecycle {
    prevent_destroy = true
  }
  cloud_id  = ycp_resource_manager_cloud.iam_service_cloud.id
  folder_id = var.iam_project_id
  name      = "iam-service"
}

resource "ycp_iam_service_account" "iam_service_account" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = var.iam_project_id
  service_account_id = "yc.iam.serviceAccount"
  name               = "iam-service-account"
}

resource "ycp_iam_service_account" "iam_common_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-common"
  service_account_id = "yc.iam.common"
  description        = "IAM common-services service account"
}

resource "ycp_iam_key" "iam_common_sa_key" {
  key_id             = ycp_iam_service_account.iam_common_sa.id
  service_account_id = ycp_iam_service_account.iam_common_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.iam_common_sa_public_key
}

resource "ycp_iam_service_account" "iam_sync_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-sync"
  service_account_id = "yc.iam.sync"
  description        = "IAM sync-account. Used to sync access bindings"
}

resource "ycp_iam_key" "iam_sync_sa_key" {
  key_id             = ycp_iam_service_account.iam_sync_sa.id
  service_account_id = ycp_iam_service_account.iam_sync_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.iam_sync_public_key
}

resource "ycp_iam_service_account" "iam_access_service_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-access-service"
  service_account_id = "yc.iam.accessService"
  description        = "IAM Access Service service account"
}

resource "ycp_iam_key" "iam_access_service_sa_key" {
  key_id             = ycp_iam_service_account.iam_access_service_sa.id
  service_account_id = ycp_iam_service_account.iam_access_service_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.access_service_public_key
}

resource "ycp_iam_service_account" "iam_control_plane_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-control-plane"
  service_account_id = "yc.iam.controlPlane"
  description        = "IAM Control Plane Service service account"
}

resource "ycp_iam_key" "iam_control_plane_sa_key" {
  key_id             = ycp_iam_service_account.iam_control_plane_sa.id
  service_account_id = ycp_iam_service_account.iam_control_plane_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.iam_control_plane_public_key
}

resource "ycp_iam_service_account" "iam_openid_server_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-openid-server"
  service_account_id = "yc.iam.openidServer"
  description        = "IAM OpenID Server service account"
}

resource "ycp_iam_key" "iam_openid_server_sa_key" {
  key_id             = ycp_iam_service_account.iam_openid_server_sa.id
  service_account_id = ycp_iam_service_account.iam_openid_server_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.openid_server_public_key
}

resource "ycp_iam_service_account" "iam_org_service_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-org-service"
  service_account_id = "yc.iam.orgService"
  description        = "IAM Organization Service service account"
}

resource "ycp_iam_key" "iam_org_service_sa_key" {
  key_id             = ycp_iam_service_account.iam_org_service_sa.id
  service_account_id = ycp_iam_service_account.iam_org_service_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.org_service_public_key
}


resource "ycp_iam_service_account" "iam_reaper_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-reaper"
  service_account_id = "yc.iam.reaper"
  description        = "IAM Reaper Service service account"
}

resource "ycp_iam_key" "iam_reaper_sa_key" {
  key_id             = ycp_iam_service_account.iam_reaper_sa.id
  service_account_id = ycp_iam_service_account.iam_reaper_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.reaper_public_key
}

resource "ycp_iam_service_account" "iam_rm_control_plane_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-rm-control-plane"
  service_account_id = "yc.iam.rmControlPlane"
  description        = "IAM Resource Manager Control Plane Service service account"
}

resource "ycp_iam_key" "iam_rm_control_plane_sa_key" {
  key_id             = ycp_iam_service_account.iam_rm_control_plane_sa.id
  service_account_id = ycp_iam_service_account.iam_rm_control_plane_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.rm_control_plane_public_key
}

resource "ycp_iam_service_account" "iam_token_service_sa" {
  lifecycle {
    prevent_destroy = true
  }
  folder_id          = ycp_resource_manager_folder.iam_service_folder.id
  name               = "yc-iam-token-service"
  service_account_id = "yc.iam.tokenService"
  description        = "IAM Token Service service account"
}

resource "ycp_iam_key" "iam_token_service_sa_key" {
  key_id             = ycp_iam_service_account.iam_token_service_sa.id
  service_account_id = ycp_iam_service_account.iam_token_service_sa.id
  key_algorithm      = "RSA_4096"
  format             = "PEM_FILE"
  public_key         = var.token_service_public_key
}
