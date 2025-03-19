terraform {
  backend "s3" {
    profile = "doublecloud-aws-preprod"
    bucket  = "datacloud-preprod-resources"
    key     = "terraform-datacloud-aws-preprod-resources"
    region  = "eu-central-1"
  }
}
