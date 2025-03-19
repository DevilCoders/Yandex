locals {
    files = [for f in var.file_secrets : {path: f.path, content: f.content, source_path: "${abspath(path.root)}/.terraform/tmp/${replace(f.path, "/", ".")}.txt"}]
}

data "external" "skm_runner" {
    depends_on = []

    program = [
        "python3",
        "${path.module}/skm_runner.py"]

    query = {
        skm_path = var.skm_path,
        yav_token = var.yav_token,
        files = jsonencode(local.files)
        src = templatefile("${path.module}/files/skm-encrypt.tpl.yaml", {
            iam_private_endpoint = var.iam_private_endpoint
            kms_private_endpoint = var.kms_private_endpoint
            cloud_api_endpoint = var.cloud_api_endpoint
            kms_key_id = var.kms_key_id
            encrypted_dek = var.encrypted_dek
            secrets = var.yav_secrets
            files = local.files
        })
    }
}


