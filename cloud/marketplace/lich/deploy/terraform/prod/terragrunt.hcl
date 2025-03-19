include "root" {
  path = find_in_parent_folders("root.hcl")
}

inputs = {
    yc_api_endpoint = "api.cloud.yandex.net:443"
    folder_id = "b1g9dapghac3s4tkhv24" # mkt-prod
}
