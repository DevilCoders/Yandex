module "jumphost_iam_frankfurt_a" {
  source = "../../../modules/infra/jumphost/v1"

  jumphost_name          = "iam-playground"
  subnet_id              = module.infra_aws_preprod.iam_frankfurt_a_public_subnet_id
  vpc_security_group_ids = [module.infra_aws_preprod.iam_frankfurt_ingress_ssh_sg_id]
  ssh_key_name           = aws_key_pair.iam_common_dev.key_name
}
