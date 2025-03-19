data "external" "skm_metadata" {
  program = [
    "python3", "${path.module}/generate.py"
  ]
  query = {
    secrets_file = "${abspath(path.module)}/secrets.yaml",

    cache_dir = "/tmp/.skm-cache/mr-prober/${var.cluster_id}/",
    iam_private_endpoint = var.iam_private_endpoint,
    kms_private_endpoint = var.kms_private_endpoint,
    key_uri = "yc-kms://${var.mr_prober_secret_kek_id}"
  }
}

locals {
  skm_metadata = data.external.skm_metadata.result.skm
}
