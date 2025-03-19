module "infra_aws_preprod" {
  source = "../../../modules/infra/v1"

  infra_name   = local.infra_name
  aws_account  = local.aws_account
  aws_profile  = local.aws_profile

  region  = local.region

  k8s_config = {
    cluster_name = "iam-${local.infra_name}"
    ssh_key_name = aws_key_pair.iam_common_dev.key_name
    node_groups  = [
      {
        name = "memory-optimized"
        instance_types     = [
          "r5.large",
          "r5a.large",
        ]
        scaling_config = {
          desired_size = 3
          max_size     = 4
          min_size     = 3
        }
      }
    ]
  }

  private_zones                 = module.constants.private_zones
  delegation_set_reference_name = "iam-preprod-common"
  public_zones                  = module.constants.public_zones
  zone_tech_iam                 = module.constants.zone_tech_iam

  iam_devops                    = local.iam_devops
  auth_ui_devops                = local.auth_ui_devops

  transit_gateways              = local.transit_gateways

  prometheus_reader_accounts    = module.constants.prometheus_reader_accounts

  # TODO Delete it
  skip_dns_public_resources     = false
  skip_metrics_resources        = false
}
