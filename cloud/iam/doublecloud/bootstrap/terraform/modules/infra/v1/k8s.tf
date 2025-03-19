module "iam_k8s" {
    source = "../../../modules/infra/k8s/v1"

    providers = {
        aws        = aws.frankfurt
        kubernetes = kubernetes.frankfurt
        helm       = helm.frankfurt
    }

    aws_account  = var.aws_account
    aws_profile  = var.aws_profile
    cluster_name = var.k8s_config.cluster_name
    ssh_key_name = var.k8s_config.ssh_key_name
    cluster_subnet_ids = local.private_subnets_ids

    node_groups = [ for ng in var.k8s_config.node_groups: {
        name  = ng.name
        subnet_ids = local.private_subnets_ids
        security_group_ids = [module.security_groups.iam_frankfurt_default_sg_id]
        labels             = {
            "yc.state"       = "ACTIVE",
            "yc.environment" = var.infra_name,
            "yc.team"        = "IAM"
        }
        instance_types     = ng.instance_types
        scaling_config      = ng.scaling_config
    }]

    cluster_encryption_config = [{
        resources        = ["secrets"]
        provider_key_arn = aws_kms_key.iam_k8s_key.arn
    }]

    ingress_cidrs      = [aws_vpc.vpc_iam_frankfurt.cidr_block]
    ingress_ipv6_cidrs = [aws_vpc.vpc_iam_frankfurt.ipv6_cidr_block]

    iam_devops         = var.iam_devops
    auth_ui_devops     = var.auth_ui_devops
}
