terraform {
  backend "s3" {
    profile = "doublecloud-aws-prod"
    bucket  = "datacloud-prod-resources"
    key     = "terraform-datacloud-aws-prod-resources"
    region  = "eu-central-1"
  }
}
