set -e

cd $(dirname "$0")

ya package --docker pkg.json
