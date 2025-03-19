DOMAINNAME=ru-central1.internal

get_metadata_value() {
  local attr=$1
  curl -s -H "Metadata-Flavor: Google" http://169.254.169.254/computeMetadata/v1/instance/$attr
}

pass() {
  echo "PASS:" "$@"
  exit 0
}

fail() {
  echo "FAIL:" "$@"
  exit 1
}

error() {
  echo "ERROR:" "$@"
  exit 2
}
