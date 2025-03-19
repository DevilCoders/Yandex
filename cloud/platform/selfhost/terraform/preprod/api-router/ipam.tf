data "terraform_remote_state" "ipam" {
  backend = "s3"

  config = {
    endpoint                    = "storage.cloud-preprod.yandex.net"
    bucket                      = "terraform-state"
    key                         = "preprod/ipam.tfstate"
    region                      = "us-east-1"
    access_key                  = "d922_h1sdxKc4oyLWEYE"
    secret_key                  = module.yav-secret-terraform-state-preprod.value
    skip_requesting_account_id  = true
    skip_credentials_validation = true
    skip_get_ec2_platforms      = true
    skip_metadata_api_check     = true
  }
}

locals {
  all_v4_addrs = data.terraform_remote_state.ipam.outputs.service-name-a_v4_address
  all_v6_addrs = data.terraform_remote_state.ipam.outputs.service-name-a_v6_address
  subnets      = data.terraform_remote_state.ipam.outputs.subnets_map
}

