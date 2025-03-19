locals {
  // https://st.yandex-team.ru/CLOUD-51142
  // https://racktables.yandex-team.ru/index.php?page=services&tab=projects&project_name=_CLOUD_MR_PROBER_TESTING_NETS_
  cloud_mr_prober_testing_nets = 64557  // == 0xfc2d
}

// Control Plane for Mr. Prober in TESTING is deployed in the PROD environment.
// Here we install only Data Plane.
module "mr_prober" {
  source = "../../modules/monitoring/mr_prober_data_plane/"

  // VLA - http://netbox.cloud.yandex.net/ipam/prefixes/1269/
  // SAS - https://netbox.cloud.yandex.net/ipam/prefixes/1262/
  // MYT - https://netbox.cloud.yandex.net/ipam/prefixes/1266/
  control_network_ipv6_cidrs = {
    ru-central1-a = cidrsubnet("2a02:6b8:c0e:2c0::/64", 32, local.cloud_mr_prober_testing_nets) 
    ru-central1-b = cidrsubnet("2a02:6b8:c02:8c0::/64", 32, local.cloud_mr_prober_testing_nets)
    ru-central1-c = cidrsubnet("2a02:6b8:c03:8c0::/64", 32, local.cloud_mr_prober_testing_nets)
  }

  dns_zone = "prober.cloud-testing.yandex.net"

  mr_prober_sa_public_key = <<EOF
-----BEGIN PUBLIC KEY-----
MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAkwrA8DEpabPGIEQ6e5Bw
TiI3inwe6dvaQC8vaZ8P/X3KsNfNG5C7oNZ+w+rMfeGXd7VduHd51jnYN6b5x3x5
0UurycC2NQ8kwIkLvRLza2Xg44smfluGXtyT0SxIiw+PKg0iIEOARC5tyQ39zKoh
4Bypgg0qFL5XhpSewDMo0rPDjB+JRG6RMZUX8E/+VPm5jJTdZj0CUXAKyvxT6HWA
emSX6pvT1uFzBOc8upPsD4JOOy2Tv+kF5RE/dfX6AeXrhswmHGrUwQJoCQ30bddd
LAH3Mlc3nJ2uMI0wVphJ+TLdaBbPIDfi4XIq2WCZ2k4wnRvic/1z6dvXpkjf1rT/
qnNw2CBJkzuKLTrojeRC5U4oJ2nRoL6u96fp9gQKCUC7kbsAufZU9oXR1gHUCxi7
EFM3zOtiUsY+bJjUCy7I892zBnSpafYsx3VeeofCgDoz8PvAaXoPG7EBbx16nBU2
gGxyskLEkz3U6gneA0DEDI+2fKXTsFA60ZQrAYr9dzyB42CIt3XL0AMTUjavSQ5e
tHwBMoq3V7wMCuJ+TY0IlhWtw8I2i0h+DuvnD/dzWVk6Zqedug0nx0rjpNkOoJa1
E9YbFva2eF9JGfPyIFoCt0bD/71lbLxng5SoMbciYkUgs2EAz2CzsPQ3SJcFEV55
bjsyNgAiXkpcbF0CmiUnDk0CAwEAAQ==
-----END PUBLIC KEY-----
EOF  
}
