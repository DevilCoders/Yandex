#!/bin/bash

# This script downloads ("mirrors") providers from Yandex Cloud's network mirror for terraform providers
# into local cache for further usage in Creator. It's a replacement for "terraform providers mirror",
# because last one can not mirror providers from our network mirror to filesystem mirror.
#
# See https://clubs.at.yandex-team.ru/ycp/4790 about https://terraform-mirror.yandexcloud.net/ —
# Yandex Cloud's network mirror for terraform providers.
#
# This scripts parses terraform_providers.tf.json, select providers stored
# at releases.hashicorp.io (they have only one "/" in "source" field)
# and download it to $FILESYSTEM_MIRROR_PATH (by default, /root/.terraform.d/plugins).
#
# Requirements: bash, jq, curl.
#
# For debugging purposes consider passing custom $FILESYSTEM_MIRROR_PATH, i.e.
# FILESYSTEM_MIRROR_PATH=tmp ./mirror_providers.sh

set -e

PROVIDERS_REQUIREMENTS_FILE=terraform_providers.tf.json
MIRROR_BASE_URL=https://terraform-mirror.yandexcloud.net/
FILESYSTEM_MIRROR_PATH="${FILESYSTEM_MIRROR_PATH:-/root/.terraform.d/plugins}"

# Iterate over required providers, filter out providers stored at releses.hashicorp.io (they have only one "/" in "source" field)
for provider in $(jq --compact-output '.terraform[0].required_providers[0] | with_entries(select(.value.source | test("^[^/]+/[^/]+$"))) | .[]' < $PROVIDERS_REQUIREMENTS_FILE)
do
  provider_source=$(echo "$provider" | jq -r .source)
  provider_version=$(echo "$provider" | jq -r .version)

  provider_mirror_folder="$FILESYSTEM_MIRROR_PATH/registry.terraform.io/$provider_source"
  mkdir -p "$provider_mirror_folder"

  # Should be something like https://terraform-mirror.yandexcloud.net/registry.terraform.io/yandex-cloud/yandex
  provider_base_url="${MIRROR_BASE_URL}registry.terraform.io/$provider_source"

  echo "Downloading $provider_base_url/index.json"
  curl --silent --fail "$provider_base_url/index.json" > "$provider_mirror_folder/index.json"

  # Should be something like https://terraform-mirror.yandexcloud.net/registry.terraform.io/yandex-cloud/yandex/0.73.0.json
  provider_url="$provider_base_url/$provider_version.json"
  echo "Downloading $provider_url"
  curl --silent --fail "$provider_url" > "$provider_mirror_folder/$provider_version.json"

  provider_linux_filename=$(jq -r .archives.linux_amd64.url < "$provider_mirror_folder/$provider_version.json")
  # Should be something like https://terraform-mirror.yandexcloud.net/registry.terraform.io/yandex-cloud/yandex/terraform-provider-yandex_0.72.0_linux_amd64.zip
  provider_linux_url="${provider_base_url}/${provider_linux_filename}"

  echo "Downloading $provider_source:$provider_version from $provider_linux_url..."
  curl --silent --fail "$provider_linux_url" > "$provider_mirror_folder/$provider_linux_filename"
done
