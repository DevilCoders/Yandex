#!/usr/bin/env bash
set -eo pipefail

case "$1" in
"help"|"--help"|"-h")
  echo "Runs 'terraform plan' and greps the changes."
  echo 'Usage:'
  echo
  echo "  $0 [target_name]"
  echo
  echo "Without arguments, runs '$ terraform plan' and exits."
  echo "Without the target name argument, runs '$ terraform plan --target target_name'"
  echo "and asks whether to apply the plan."
  echo 
  echo "Saves the binary '/tmp/.../terraform_plan.bin' with actions to apply,"
  echo "and the full plan log in '/tmp/.../terraform_plan.log'."
  echo
  echo "Tries hard to detect the 'yc_token' and 'yandex_token' terraform variables:"
  echo ' - checks the $TF_VAR_yc_token and $TF_VAR_yandex_token env. variables,'
  echo ' - checks the $YC_TOKEN and $YT_TOKEN env. variables,'
  echo " - looks in the ~/terraform.tfvars file."
  echo "(Doesn't support the -var and -var-file args.)"
  echo
  echo "Also see https://a.yandex-team.ru/arc/trunk/arcadia/cloud/platform/selfhost/terraform"
  exit 0
  ;;
esac

###############################################################################
# 1. Detect the 'yc_token' and 'yandex_token' terraform variables.
#
# They can either be in the TF_VAR_* env. variables, in the Y*_TOKEN variables,
# or in the ~/terraform.tfvars file.
set +x

if [[ -z "$TF_VAR_yc_token" ]]; then
  export TF_VAR_yc_token=$YC_TOKEN
fi
if [[ -z "$TF_VAR_yc_token" ]]; then
  export TF_VAR_yc_token=$(grep yc_token ~/terraform.tfvars 2> /dev/null | awk -F\" '{print $2}')
fi
if [[ -z "$TF_VAR_yc_token" ]]; then
  echo "Couldn't find the 'yc_token' variable."
  echo "See https://a.yandex-team.ru/arc/trunk/arcadia/cloud/platform/selfhost/terraform#usage"
  exit 1
fi

if [[ -z "$TF_VAR_yandex_token" ]]; then
  export TF_VAR_yandex_token=$YT_TOKEN
fi
if [[ -z "$TF_VAR_yandex_token" ]]; then
  export TF_VAR_yandex_token=$(grep yandex_token ~/terraform.tfvars 2> /dev/null | awk -F\" '{print $2}')
fi
if [[ -z "$TF_VAR_yandex_token" ]]; then
  echo "Couldn't find the 'yandex_token' variable."
  echo "See https://a.yandex-team.ru/arc/trunk/arcadia/cloud/platform/selfhost/terraform#usage"
  exit 1
fi

###############################################################################
# 2. Plan!
#
set -x

TMP_DIR=$(mktemp -d)

# Check if we're running plan for everything or for a single target.
TARGET_ARG=""
if [[ ! -z "$1" ]]; then
  TARGET_ARG="--target $1"
fi

terraform plan $TARGET_ARG -out $TMP_DIR/terraform_plan.bin > $TMP_DIR/terraform_plan.log

tail $TMP_DIR/terraform_plan.log
{ echo -e "------------------------------------------------------------------------\n"; } 2> /dev/null
cat $TMP_DIR/terraform_plan.log | grep -2 -E '[[]3[0-7]m[-+~]'

###############################################################################
# 3. Apply! (Or not.)
set +x

# Do nothing unless there is a single target.
if [[ -z "$TARGET_ARG" ]]; then
  echo
  echo "============================================================"
  echo "In order to apply these changes, run the following commands:"
  echo
  terraform state list | grep -F yandex_compute_instance | xargs -n1 echo "  $ $0"
  echo
  echo "(also see 'terraform state list')"
  exit 0
fi

# Ask before running apply.
read -p 'Type "yes" to apply: ' -r
if [[ "$REPLY" != "yes" ]]; then
  echo
  echo "Run the following command to apply:"
  echo
  echo "  $ terraform apply $TMP_DIR/terraform_plan.bin"
  echo
  exit 0
fi

# Apply the saved plan.
set -x
terraform apply $TMP_DIR/terraform_plan.bin
