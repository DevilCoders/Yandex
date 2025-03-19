terraform {
  required_providers {
    # Update on Linux:
    # $ curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
    # Update on Mac:
    # $ brew upgrade terraform-provider-ycp
    ycp = ">= 0.7"
  }
}

data "external" "token" {
  program = ["/bin/bash", "-c", "yc config get token | jq -Rn '{token: input}'"]
}

provider "ycp" {
  prod        = false
  token       = data.external.token.result.token
  cloud_id    = var.cloud_id
  folder_id   = var.folder_id
  ycp_profile = "testing"
  ycp_config  = "../ycp-config.yaml"
}

resource ycp_iam_service_account main {
  # id = d26lefh490k7jnbt4a4j
  name = "main"
}

resource ycp_resource_manager_folder_iam_member main-sa-editor {
  folder_id = var.folder_id
  role      = "editor"
  member    = "serviceAccount:${ycp_iam_service_account.main.id}"
}

# IG df2oluo8u9i2h7636o64
module "cpl" {
  source = "./modules/envoy"

  name            = "cpl"
  tracing_service = "cpl-router"
  server_cert_pem = file("cpl/server.pem")
  server_cert_key = file("cpl/server_key.json")
  alb_lb_id       = "albpufdu7qaneuf4t8bg"
  ig_sa           = ycp_iam_service_account.main.id
  instance_sa     = ycp_iam_service_account.main.id
}

#IG df2421nf0cn97h4e3hra
module "api" {
  source = "./modules/envoy"

  name            = "api-router"
  tracing_service = "api-router"
  server_cert_pem = file("api-router/server.pem")
  server_cert_key = file("api-router/server_key.json")
  alb_lb_id       = "alb6fbilfn53niu6738l"
  ig_sa           = ycp_iam_service_account.main.id
  instance_sa     = ycp_iam_service_account.main.id
}
