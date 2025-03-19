export PATH="/root/ycp/bin:$PATH"

function get_iam_token {
    # https://st.yandex-team.ru/CLOUD-97637
    # This functions extracts stand SA's private key from Lockbox,
    # converts it to IAM-token (via JWT) and returns it.
    LOCKBOX_SECRET_ID="$1"
    PROFILE_NAME="$2"

    SCRIPTS_DIR=$(dirname $(realpath "$BASH_SOURCE"))

    # Find current version of lockbox secret
    LOCKBOX_VERSION_ID=$(ycp --profile prod lockbox v1 secret get "$LOCKBOX_SECRET_ID" --format json | jq -r .current_version.id)
    # Get text content of this version (assume that it stores the private key)
    PRIVATE_KEY=$(ycp --profile prod lockbox v1 payload get --secret-id "$LOCKBOX_SECRET_ID" --version-id "$LOCKBOX_VERSION_ID" --format json | jq -r ".entries[] | select(.key == \"$PROFILE_NAME\").text_value")

    # Convert Authorized Key to JWT
    JWT=$("$SCRIPTS_DIR/generate_jwt_by_authorized_key.py" yc.devel.yc-monitoring.sa yc.devel.yc-monitoring.sa.key <<< "$PRIVATE_KEY")

    # Get IAM token by JWT
    ycp --profile "$PROFILE_NAME" iam iam-token create --jwt "$JWT"
}
