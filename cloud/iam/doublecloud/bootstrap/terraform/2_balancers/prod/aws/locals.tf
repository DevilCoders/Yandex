locals {
  provider   = "aws"
  aws_account = "937784353709"
  infra = {
    name = "prod"
    regions = {
      frankfurt = {
        vpc = "vpc-0b6034c3f2a3b85c7"            # VPC iam-frankfurt
        security_group = "sg-0b65161b384573fc0"  # SG IAM prod default frankfurt
        private_subnets = [
          "subnet-0eb2b680180cd01e6",            # iam_frankfurt_a_private
          "subnet-07c53844207588107",            # iam_frankfurt_b_private
          "subnet-04cd7008fd3141446",            # iam_frankfurt_c_private
        ]
        public_subnets = [
          "subnet-0470467d823a37fed",            # iam_frankfurt_a_public
          "subnet-0da51368583372f6a",            # iam_frankfurt_b_public
          "subnet-0982f290bb4d693c5",            # iam_frankfurt_c_public
        ]
      }
    }
  }

  k8s_cluster_name    = "iam-${local.infra.name}"

  team_aws_accounts = [
    "710586422240", # Common Infra
    "576927435645", # Billing
    "883433064081", # MDB
    "221736954043", # ClickHouse
    "288063047574", # Datalens-UI
    "179772598917", # DataTransfer
    "511453442829", # YDB
  ]
}

module "constants" {
  source = "../../../modules/general/constants/aws-prod"
}
