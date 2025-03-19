set -e

cd `dirname $0`

if [ ${USE_ENV_TOOLS:-0} -eq 0 ]; then
    export YC_TOKEN=`yc iam create-token`
fi

if [ "${SKM_TOKEN}" ]; then
    export YC_TOKEN="${SKM_TOKEN}"
fi

skm decrypt-bundle --bundle skm-encrypted-keys.yaml --no-chown
