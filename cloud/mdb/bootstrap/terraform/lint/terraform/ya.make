PACKAGE()

OWNER(g:mdb)

# Execute following commands and update FROM_SANDBOX macro with correct IDs:
#
# export TF_VERSION=<correct terraform version>
# curl https://releases.hashicorp.com/terraform/${TF_VERSION}/terraform_${TF_VERSION}_linux_amd64.zip -o tf.zip
# unzip tf.zip
# ya upload --ttl=inf --tar terraform
# rm -rf tf.zip terraform
# curl https://releases.hashicorp.com/terraform/${TF_VERSION}/terraform_${TF_VERSION}_darwin_amd64.zip -o tf.zip
# unzip tf.zip
# ya upload --ttl=inf --tar terraform
# rm -rf tf.zip terraform

IF(OS_LINUX)
    FROM_SANDBOX(2021965651 OUT_NOAUTO terraform EXECUTABLE)
ELSEIF(OS_DARWIN)
    FROM_SANDBOX(2021962596 OUT_NOAUTO terraform EXECUTABLE)
ENDIF()

END()
