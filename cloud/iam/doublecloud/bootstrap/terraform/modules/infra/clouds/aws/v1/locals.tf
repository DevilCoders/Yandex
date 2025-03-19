locals {
  regions = {
    frankfurt = {
      id         = "eu-central-1"
      human_name = "frankfurt"
      // Name <-> id is randomized per AWS account. We should not rely on it. We even use generated zone name.
      // https://docs.aws.amazon.com/ram/latest/userguide/working-with-az-ids.html
      zones = {
        a = {
          id = "euc1-az2"
        }
        b = {
          id = "euc1-az3"
        }
        c = {
          id = "euc1-az1"
        }
      }

      vpc_cidr = "10.96.0.0/16"
    }
  }

  tags = {
    discounts = {
      all = {
        name  = "map-migrated"
        value = "d-server-01fsq5a96hp704"
      },
      rds = {
        name  = "map-dba"
        value = "TDB"
      }
    }
  }
}
