#!/bin/sh
ya upload \
    --sandbox-mds \
    --do-not-remove \
    --owner CI \
    --description 'ci goxcart configurations' \
    --type CI_API_GOXCART_CONFIG \
    config
