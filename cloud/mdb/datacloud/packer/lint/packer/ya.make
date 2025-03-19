PACKAGE()

OWNER(g:mdb)

# Execute following commands and update FROM_SANDBOX macro with correct IDs:
#
# export PACKER_VERSION=<correct packer version>
# curl "https://releases.hashicorp.com/packer/${PACKER_VERSION}/packer_${PACKER_VERSION}_linux_amd64.zip" -o packer.zip
# unzip packer.zip
# ya upload --ttl=inf --tar packer
# rm -rf packer.zip packer
# curl "https://releases.hashicorp.com/packer/${PACKER_VERSION}/packer_${PACKER_VERSION}_darwin_amd64.zip" -o packer.zip
# unzip packer.zip
# ya upload --ttl=inf --tar packer
# rm -rf packer.zip packer

IF(OS_LINUX)
    FROM_SANDBOX(2750318077 OUT_NOAUTO packer EXECUTABLE)
ELSEIF(OS_DARWIN)
    FROM_SANDBOX(2750325940 OUT_NOAUTO packer EXECUTABLE)
ENDIF()

END()
