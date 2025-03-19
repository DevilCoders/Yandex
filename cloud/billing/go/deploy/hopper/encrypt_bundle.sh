set -xe

export YC_TOKEN=`yc iam create-token`
export YAV_TOKEN=`yav oauth`

skm encrypt-bundle --bundle skm-encrypted-keys.yaml
# skm decrypt-bundle --bundle skm-encrypted-keys.yaml
