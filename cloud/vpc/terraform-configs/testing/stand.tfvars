ycp_profile = "testing"
yc_endpoint = "api.cloud-testing.yandex.net:443"
environment = "testing"
# There is no Container Registry on the Testing
cr_endpoint = "cr.cloud-testing.yandex.net"
hc_network_ipv6 = "2a0d:d6c0:2:ba:1::/80"
# There is no LogBroker on the Testing, so specify endpoint as on the Preprod
logbroker_endpoint = "lb.cc8035oc71oh9um52mv3.ydb.mdb.cloud-preprod.yandex.net"
logbroker_database = "/pre-prod_global/aoeb66ftj1tbt1b2eimn/cc8035oc71oh9um52mv3"
region_id = "ru-central1"
availability_zones = ["ru-central1-a", "ru-central1-b", "ru-central1-c"]
