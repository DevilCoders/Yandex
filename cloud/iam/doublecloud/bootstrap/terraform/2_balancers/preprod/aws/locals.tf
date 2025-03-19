locals {
  provider   = "aws"
  aws_account = "821159050485"
  infra = {
    name = "preprod"
    regions = {
      frankfurt = {
        vpc = "vpc-01da586bf2dbda052"            # VPC iam-frankfurt
        security_group = "sg-0852f3d5b005dc6bf"  # SG IAM preprod default frankfurt
        private_subnets = [
          "subnet-03c6f30e416943228",            # iam_frankfurt_a_private
          "subnet-02f4f46e480db2425",            # iam_frankfurt_b_private
          "subnet-0be281878445b99b6",            # iam_frankfurt_c_private
        ]
        public_subnets = [
          "subnet-0a6316bd6528de81d",            # iam_frankfurt_a_public
          "subnet-08a6d12b27d2d7828",            # iam_frankfurt_b_public
          "subnet-0d74c7f36541f338a",            # iam_frankfurt_c_public
        ]
      }
    }
  }

  k8s_cluster_name    = "iam-${local.infra.name}"

  team_aws_accounts = [
    "785415555008", # Common Infra
    "411829009811", # Billing
    "690619578286", # YDB
  ]
}

module "constants" {
  source = "../../../modules/general/constants/aws-preprod"
}
