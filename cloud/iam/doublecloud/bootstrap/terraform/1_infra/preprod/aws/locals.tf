locals {
  infra_name = "preprod"
  aws_account = "821159050485"

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
      transit_gateway_id = "tgw-08e91f577e9fd88cb"
      cidr_block         = "10.100.0.0/16"
    },
  ]
}

module "constants" {
  source = "../../../modules/general/constants/aws-preprod"
}
