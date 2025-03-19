terraform {
  required_providers {
    # Update on Linux:
    # $ curl https://mcdev.s3.mds.yandex.net/terraform-provider-ycp/install.sh | bash
    # Update on Mac:
    # $ brew upgrade terraform-provider-ycp
    ycp = ">= 0.35"
  }
}

provider "ycp" {
  prod      = false
  cloud_id  = "aoe0oie417gs45lue0h4"
  folder_id = "aoe4lof1sp0df92r6l8j"
}
