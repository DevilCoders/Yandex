module "balancer" {
  source = "../../../modules/balancer/v1"

  aws_profile                      = local.aws_profile
  k8s_cluster_name                 = local.k8s_cluster_name

  cloud_provider                   = local.provider
  env_name                         = local.infra.name
  infra                            = local.infra
  iam_default_security_group_id    = local.infra.regions.frankfurt.security_group
  region_suffix                    = "private-api.eu-central-1.aws.preprod.iam"

  fqdn_internal_iam                = module.constants.fqdn_internal_iam
  # TODO Remove it
  fqdn_internal_iam_alternatives   = ["iam.internal.yadc.io"]
  zone_internal_iam                = module.constants.zone_internal_iam
  fqdn_operations                  = module.constants.fqdn_operations
  fqdn_public_auth                 = module.constants.fqdn_public_auth
  zone_public_auth                 = module.constants.zone_public_auth
  zone_private_iam                 = module.constants.zone_private_iam

  endpoint_service_acceptance_required = false
  endpoint_service_allowed_principals  = [ for acc_id in local.team_aws_accounts : "arn:aws:iam::${acc_id}:root" ]

  fqdn_notify                      = module.constants.fqdn_notify
  zone_notify                      = module.constants.zone_private_legacy
  notify_endpoint_service_name     = module.constants.notify_endpoint_service_name

  billing_endpoint_fqdn            = module.constants.billing_endpoint_fqdn
  billing_endpoint_service_name    = module.constants.billing_endpoint_service_name

  zone_mdb                         = module.constants.zone_private_legacy
  mdb_endpoint_fqdn                = module.constants.mdb_endpoint_fqdn
  mdb_endpoint_service_name        = module.constants.mdb_endpoint_service_name

  zone_vpc                         = module.constants.zone_private_legacy
  vpc_endpoint_fqdn                = module.constants.vpc_endpoint_fqdn
  vpc_endpoint_service_name        = module.constants.vpc_endpoint_service_name
}
