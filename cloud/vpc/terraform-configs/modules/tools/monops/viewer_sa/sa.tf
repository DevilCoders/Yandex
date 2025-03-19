// Module for creating monops service account with viewer permissions
// (whose JWT token used by PROD monops instance).

resource "ycp_iam_service_account" "monops_viewer_sa" {
  lifecycle {
    prevent_destroy = true
  }

  name        = "monops-viewer"
  description = "service account to view vpc-related data"
  folder_id   = var.folder_id
}
