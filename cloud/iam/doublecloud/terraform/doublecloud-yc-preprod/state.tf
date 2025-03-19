terraform {
  backend "s3" {
    endpoint   = "s3.mds.yandex.net"
    bucket     = "yc-iam"
    key        = "terraform-datacloud-yc-resources"
    region = "us-east-1"
    // Access key for @robot-yc-iam access is used.
    // secret_key is stored in yav.yandex-team.ru and must be passed to terraform via command line argument:
    // `terraform init -backend-config="secret_key=$(ya vault get version sec-01ewn2b4vf3vmj18c65w1b8wh6 -o AccessSecretKey)"`
    access_key = "J2Pflp34mNvO25F977j6"
    skip_credentials_validation = true
    skip_metadata_api_check     = true
  }
}
