#!/bin/bash

if ! handles="$(yc-tpm-agent tpm-getcap handles-persistent list)" ; then
  echo "Can't check a handles from the TPM. Exit."
  exit 1
fi

if echo "${handles}" | grep -q "0x" ; then
  echo -e "The TPM is already initialized. Handles:\n${handles}"
else
  echo "No any handles created yet. Trying to init the TPM ..."
  yc-tpm-agent tpm-init
fi
