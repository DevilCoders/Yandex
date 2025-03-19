locals {
  infra_name = "prod"
  aws_account = "937784353709"

  region = "frankfurt"

  iam_devops = [
    "karelinoleg",
    "pbludov",
    "potamus",
  ]

  auth_ui_devops = [
    "slywyrm",
  ]

  transit_gateways = [
    {
      transit_gateway_id = "tgw-076d54147de1a4f56"
      cidr_block         = "10.100.0.0/16"
    },
  ]
}

module "constants" {
  source = "../../../modules/general/constants/aws-prod"
}
