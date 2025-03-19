locals {
  delegation_set_reference_name = "iam-preprod-common"
  zone_internal_iam             = "iam.internal.yadc.io"
  public_zones                  = [
    local.zone_internal_iam,
  ]

  provider   = "aws"
  aws_account = "785415555008"
  infra = {
    name = "preprod"
    regions = {
      frankfurt = {
        vpc = "vpc-06a4cfaac8e5e0322"            # VPC iam-frankfurt
        security_group = "sg-01f1706bf816b723b"  # SG IAM preprod default frankfurt
        private_subnets = [
          "subnet-0421eec1ec9b8679c",            # iam_frankfurt_a_private
          "subnet-04040800b8db45d26",            # iam_frankfurt_b_private
          "subnet-059d567cf1286fb35",            # iam_frankfurt_c_private
        ]
        public_subnets = [
          "subnet-05cdff2fafb886bda",            # iam_frankfurt_a_public
          "subnet-0c106a1ee9afa4bc4",            # iam_frankfurt_b_public
          "subnet-0c12c13e4cc051e36",            # iam_frankfurt_c_public
        ]
      }
    }
  }
}
