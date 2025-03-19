set -e

cd $(dirname "$0")

eval $(minikube -p minikube docker-env)

BUILD='ya package --docker --docker-registry=for-minikube'

$BUILD sql/pkg.json

$BUILD ydb-client/pkg.json

$BUILD ydb-server/pkg.json
