module "yav_secret" {
    source = "../yav_secret"
    value_name = var.yav_value_name
    id = var.yav_id
    yt_oauth = var.yav_token
}

locals {
    key_id = jsondecode(module.yav_secret.secret)["id"]
    sa_id = jsondecode(module.yav_secret.secret)["service_account_id"]
}

data "template_file" "solomon-agent" {
    template = file("${path.module}/files/solomon-agent.conf")
    vars = {
        path = var.config_path
        service = var.service
        project = var.project
        service_account_id = local.sa_id
        sa_key_id = local.key_id
        sa_public_key_path = var.sa_public_key_path
        sa_private_key_path = var.sa_private_key_path
        shard_service = var.shard_service
        cluster = var.cluster
        loglevel = var.loglevel
    }
}

data "template_file" "solomon-agent-extra" {
    template = file("${path.module}/files/solomon-agent-extra.conf")
    vars = {
        service = var.service
        project = var.project
    }
}
