
terraform {
  required_version = ">= 0.12"
  required_providers {
    # Update on Linux:
    # $ curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
    # Update on Mac:
    # $ brew upgrade terraform-provider-ycp
    ycp = ">= 0.9"
  }
}
