#!/usr/local/bin/bash

TIERS="stag=RusTier0"
#TIERS="stag=RusTier0 stag=EngTier0"

sky testshards | awk '{print $1}' | grep "^[fw]"
#bsconfig ilookup itag=testws-production-replica $TIERS | awk -F: '{print $1}' | awk '{print $2}' | grep "^[f|w].*"
