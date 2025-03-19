include "root" {
  path = find_in_parent_folders("root.hcl")
}

inputs = {
    yc_api_endpoint = "api.cloud-preprod.yandex.net:443"
    folder_id = "aoenutk0inmh194jo9sr" # mkt-prod
}
