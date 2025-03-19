locals {
  hbf_environments = {
    testing = true
    preprod = true
    prod    = true

    gpn               = false
    "private-testing" = false
  }

  dc_short = {
    ru-central1-a = "rc1a"
    ru-central1-b = "rc1b"
    ru-central1-c = "rc1c"
  }
}
