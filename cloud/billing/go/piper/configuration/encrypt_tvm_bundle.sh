set -xe

cd configs/$1/tvm

export YC_TOKEN=`yc iam create-token`

skm encrypt-bundle --bundle skm-encrypted-keys.yaml
# skm decrypt-bundle --bundle skm-encrypted-keys.yaml
