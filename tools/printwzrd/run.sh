#!/bin/bash

# This script checkouts and builds all that is needed for printwizard and launches it with the input 'input.txt'
[[ "$ARCADIA" ]] || {
    ARCADIA="$(readlink -f "$BASH_SOURCE")"
    ARCADIA="${ARCADIA%/tools*}"
}

[[ -d "$ARCADIA" ]] || { echo -e "\e[31;1m[ FAIL ] Failed to autodetect the ARCADIA root, set it via environment, like this:\n export ARCADIA=~/arcadia\e[0m"; exit 1; }

readonly SCRIPTS_DIR="$ARCADIA/web/daemons/begemot/scripts"
[[ -e "$SCRIPTS_DIR/util.h.sh" ]] || {
    svn status --depth=empty |& grep 'not a working copy' 2>/dev/null || svn up --parents --set-depth infinity "$SCRIPTS_DIR"
    [[ -e "$SCRIPTS_DIR/util.h.sh" ]] || { echo -e "\e[31;1m[ FAIL ] Failed to load \"$SCRIPTS_DIR/util.h.sh\". Validate your working copy.\e[0m"; exit 1; }
}
. "$SCRIPTS_DIR/util.h.sh"

warn 'This script is no longer supported and may fail.
         Consider using web/daemons/begemot/test_begemot.sh (as `bgtest begemot` and then `bgtest request`)
         or pass the result of `ya make web/daemons/begemot` to GET_BEGEMOT_RESPONSES in sandbox (along with printwzrd.txt path and built shard).'

if [[ ${YAMAKE_ARGS+set} ]]; then
    note "Using YAMAKE_ARGS='$YAMAKE_ARGS'. Run 'unset YAMAKE_ARGS' to revert them to default"
else
    YAMAKE_ARGS=-r # build in release mode by default
fi

CONFIG=configs/wizard-yaml.cfg
DATA="$ARCADIA/search/wizard/data"
DATA_WIZARD="$DATA/wizard"


if [[ "$1" = "--geo" ]]; then
    CONFIG=configs/geo-yaml.cfg
    GEO_CGI="--add-cgi geoaddr_geometa=1&rn=Geosearch"
    shift
fi

if [[ "$1" ]]; then
    input_flag="--input $1"
    shift
else
    note 'Reading queries from stdin...'
fi

note "Run 'export QUIET=1' in advance to disable all these messages"
# The steps are ordered by the expected build time, the fastest — ther first
progress_steps=4
log_step "Generating configs..."
#yamake configs $YAMAKE_ARGS || die "Failed to generate configs"
log_step "Building the printwizard binary..."
#yamake $YAMAKE_ARGS || die "Failed to build printwizard"
log_step "Building wizard data..."
#yamake $YAMAKE_ARGS "$DATA_WIZARD" || die "Failed to build wizard data"
log_step "Starting printwizard..."
note ./printwzrd -s "$CONFIG" -a "$DATA" $input_flag "$@" $GEO_CGI '|& tee printwizard.out'
./printwzrd -s "$CONFIG" -a "$DATA" $input_flag "$@" $GEO_CGI |& tee printwizard.out
