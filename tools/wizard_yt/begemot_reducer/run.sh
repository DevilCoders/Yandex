#!/usr/bin/env bash

ORIG_OPTIONS=("$@")

[[ "$ARCADIA" ]] || {
    ARCADIA="$(readlink -f "$BASH_SOURCE")"
    ARCADIA="${ARCADIA%/tools/wizard_yt*}"
}

[[ -d "$ARCADIA" ]] || { echo -e "\e[31;1m[ FAIL ] Failed to autodetect the ARCADIA root, set it via environment, like this:\n export ARCADIA=~/arcadia\e[0m"; exit 1; }

readonly SCRIPTS_DIR="$ARCADIA/web/daemons/begemot/scripts"
. "$SCRIPTS_DIR/util.h.sh" || {
    (( "$(svn status --depth=empty |& wc -l)" == 0 )) && svn up --parents --set-depth infinity "$SCRIPTS_DIR"
    . "$SCRIPTS_DIR/util.h.sh" || { echo -e "\e[31;1m[ FAIL ] Failed to load \"$SCRIPTS_DIR/util.h.sh\". Validate your working copy.\e[0m"; exit 1; }
}

all_shards=("wizard")
for i in $(ls -d $ARCADIA/search/begemot/data/*/); do
    all_shards+=($(basename ${i%%/}));
done

test_shards=()
test_merger=
declare -A BEGEMOT_CONFIG

type getopt &>/dev/null && {
    eval set -- "$(getopt -o gswndhi:c: --long input:,debug,help,no-mapper,skip-build,wizard,cgi,config: -- "$@")"
    while [[ $1 ]]; do
        case ${all_shards[@]} in
            *$1*)
                test_shards+=($1)
                if [[ $1 == Merger ]]; then
                    test_merger=true
                fi
                shift
                continue
        esac
        case $1 in
            -i | --input)
                INPUT=$2
                shift 2
                ;;
            -n | --no-mapper)
                NO_MAPPER=true
                shift
                ;;
            -c | --config)
                CONFIG=$2
                note "CONFIG=$CONFIG"
                shift 2
                ;;
            -d | --debug)
                DEBUG=true
                shift
                ;;
            -w | --wizard)
                WIZARD=true
                shift
                ;;
            -s | --skip-build)
                SKIP_BUILD=true
                shift
                ;;
            -g | --cgi)
                CGI_MODE=true
                shift
                ;;
            -h | --help)
                echo "Usage $0 --input INPUT_TABLE Grunwald Bravo Merger"
                echo "Options:"
                echo "-i, --input           path to input table"
                echo "-u, --update-shard    enables shard updating"
                echo "-c, --config          file with workers apphost config"
                echo "-d, --debug           prints commands without running"
                echo "-w, --wizard          run with wizard"
                echo "-g, --cgi             run reducer in cgi mode, mapper on table with columns text, lr"
                echo "--no-mapper           disables logs preprocessing (input table is expected to be processed)"
                exit 0
                ;;
            *) shift ;;
        esac
    done
}

init() {
    note "ARCADIA: $ARCADIA"
    [[ $USER ]] || die "user is not set"

    local YT_PATH_PREFIX="//home/search-runtime/$USER/begemot-yt"
    [[ $INPUT ]] || die "Input table must be provided"
    [[ $OUTPUT ]] || OUTPUT="$YT_PATH_PREFIX/reducer-output/run-$RANDOM"

    note "All shards: ${all_shards[@]}"

    [[ $YT_TOKEN ]] || die "YT_TOKEN is not set"
    [[ $YT_PROXY ]] || {
        YT_PROXY="hahn.yt.yandex.net"
        note "YT proxy is not set, hahn will be used"
    }
    [[ $SHARD_CACHE ]] || SHARD_CACHE="$YT_PATH_PREFIX/shards_cache"
    [[ $SHARD_PATH ]] || { SHARD_PATH="$OUTPUT/shards"; UPDATE_SHARD=true; }
    [[ $THREADS ]] || THREADS=20
    [[ $JOBS ]] || JOBS=20
    [[ $CACHE_SIZE ]] || CACHE_SIZE=0

    only_merger=
    shards_count=${#test_shards[@]}
    if [[ $test_merger && $shards_count == 1 && -z $WIZARD ]]; then
        only_merger=true
    fi
    note "SHARD_CACHE=$SHARD_CACHE"
    note "CACHE_SIZE=$CACHE_SIZE"
    note "THREADS=$THREADS"
    note "SEQUENTIAL=$SEQUENTIAL (set non-empty for running workers sequentially)"

    REDUCER_REVISION=$(svn info "$ARCADIA/tools/wizard_yt/begemot_reducer" | grep Revision | awk '{print $2}')
    note "Reducer revision: $REDUCER_REVISION"

    SHARD_REVISION=$(svn info "$ARCADIA/search/begemot/data" | grep Revision | awk '{print $2}')
    note "Shard revision: $SHARD_REVISION"

    note "Shards to test: ${test_shards[@]}"
    hint "SHARD_PATH=$SHARD_PATH (export this env variable to use this shard next time)"
    local proxy=${YT_PROXY%.yt.yandex.net}
    hint "OUTPUT=$OUTPUT (https://yt.yandex-team.ru/$proxy/#page=navigation&path=$OUTPUT)"

    local yt_tools="$ARCADIA/tools/wizard_yt"
    updater_path="$yt_tools/shard_updater"
    merger_path="$yt_tools/merger"
    begemot_reducer_path="$yt_tools/begemot_reducer"
    wizard_mapper_path="$yt_tools/wizard_reducer"

    if [[ -a $CONFIG ]]; then
        for shard in ${test_shards[@]}; do
            local cfg=$(cat $CONFIG | jq ".$shard")
            if [[ $cfg == null ]]; then
                continue
            fi
            note $shard begemot_config: $cfg
            BEGEMOT_CONFIG[$shard]=$cfg
        done
    elif [[ $CONFIG ]]; then
        die "config file $CONFIG not found"
    fi
}

build_binaries() {
    if [[ $WIZARD ]]; then
        log_step_wrapper "Generating wizard config"
        generate_wizard_config
        log_step_wrapper "Building wizard reducer"
        run yamake $wizard_mapper_path --musl || die "Cannot build merger"
    fi
    log_step_wrapper "Building begemot reducer and tools"
    run yamake $begemot_reducer_path --musl || die "Cannot build begemot reducer"
    run yamake $updater_path --musl || die "Cannot build shard updater"
    run yamake $merger_path --musl || die "Cannot build merger"
}

generate_wizard_config() {
    CONF_PATH="$ARCADIA/search/wizard/data/wizard/conf"

    [[ -d "$CONF_PATH" ]] || svn up --set-depth infinity "$CONF_PATH" || die "The folder «${CONF_PATH}» is not checkouted, but is necessary to run wizard. Please, checkout it."
    run yamake -r "$CONF_PATH/executable" || die "Failed to generate configs"
    run "$CONF_PATH/executable/executable" --basedir "$CONF_PATH" --quiet --shard-prefix wizard/WIZARD_SHARD --printwizard ./wizard-config/ || die "Failed to generate production configs"
}

build_shards() {
    for shard in ${test_shards[@]}; do
        log_step_wrapper "Building shard $shard"
        run yamake "$ARCADIA/search/begemot/data/$shard" || die "Cannot build shard $shard"
    done
    if [[ $WIZARD ]]; then
        log_step_wrapper "Building wizard data"
        run yamake "$ARCADIA/search/wizard/data/wizard" || die "Cannot build shard $shard"
    fi
}

run_updater() {
    run "$updater_path/shard_updater" \
        --cache-path $SHARD_CACHE \
        --data $1 \
        --output "$SHARD_PATH/$shard" \
        --ttl $ttl \
        --jobs 25
}

update_shards() {
    local ttl=$(python2 -c "import datetime; print (datetime.datetime.now() + datetime.timedelta(days=30)).isoformat()")
    note "shards ttl: $ttl"
    local updater_path="$ARCADIA/tools/wizard_yt/shard_updater"
    for shard in ${test_shards[@]}; do
        log_step_wrapper "Updating shard $shard"
        run_updater "$ARCADIA/search/begemot/data/$shard/search/wizard/data/wizard"
    done
    if [[ $WIZARD ]]; then
        local shard="wizard"
        log_step_wrapper "Updating shard $shard"
        run_updater "$ARCADIA/search/wizard/data/wizard"
    fi
}

run_mapper() {
    if [[ ! $CGI_MODE ]]; then
        log_step_wrapper "Running mapper"
         run "$begemot_reducer_path/eventlog_mapper/eventlog_mapper" \
            --input $1 \
            --output $2
    fi
}

run_begemot_mapper() {
    local shard=$1
    local input_table=$2
    log_step_wrapper "Running worker $shard"
    run yt create map_node $OUTPUT/$shard -ri
    local in_background=true
    local threads=$THREADS
    [[ ${shard} = "Merger" ]] && threads=5
    local args=("$begemot_reducer_path/mapper/mapper" \
        --input ${input_table} \
        --output "$OUTPUT/${shard}/answers" \
        --cypress_shard "$SHARD_PATH/$shard" \
        --threads ${threads} \
        --job_count ${JOBS} \
        --shard $shard)
    [[ $CGI_MODE && $NO_MAPPER ]] && args+=(--direct '"text,lr"')
    [[ $CACHE_SIZE ]] && args+=(--cache_size $CACHE_SIZE)
    [[ $config ]] && args+=(--begemot_config "'$config'")
    run ${args[@]}
}

run_wizard_mapper() {
    warn "Protobuf mode is no longer supported in wizard, Merger won't use its result, use begemot instead of wizard"
    local input_table=$1
    log_step_wrapper "Running wizard"
    local result_type_opt=
    local in_background=true
    run "$wizard_mapper_path/mapper/mapper" \
        --input $input_table \
        --output "$OUTPUT/wizard_answers" \
        --cypress_shard "$SHARD_PATH/wizard" \
        --threads $THREADS \
        --job_count $JOBS \
        --config_file "./wizard-config/wizard-yaml.cfg" \
        --apphost
}

merge_answers() {
    log_step_wrapper "Merging workers answers"
    local args=($@)
    local output=${args[-1]}
    unset args[-1]
    local inputs=()
    for table in ${args[@]}; do
        inputs+=("--input")
        inputs+=($table)
    done
    run "$merger_path/merger" \
        ${inputs[@]} \
        --output "$output"
}

log_step_wrapper() {
    if [[ $INCREMENT_MODE ]]; then
        ((++progress_steps))
    else
        log_step $1
    fi
}

run() {
    [[ -z $INCREMENT_MODE ]] || return 0
    note $@
    [[ -z $DEBUG ]] || return 0
    if [[ -z $in_background || $SEQUENTIAL ]]; then
        eval $@ || die "Failed to run $@"
    else
        note "run in background"
        (eval $@ || (err "$@" && err "Failed to run $@. All processes are terminated" && kill 0)) &
    fi
}

main() {
    [[ $SKIP_BUILD ]] || build_binaries

    if [[ $UPDATE_SHARD ]]; then
        build_shards
        update_shards
    fi

    local input=$INPUT
    if [[ -z $NO_MAPPER ]]; then
        input="$OUTPUT/prepared_requests"
        run_mapper $INPUT $input
    fi

    local answers=()
    if [[ $WIZARD ]]; then
        run_wizard_mapper $input
        answers+=("$OUTPUT/wizard_answers")
    fi

    for shard in ${test_shards[@]}; do
        [[ $shard == "Merger" ]] && continue
        local config=${BEGEMOT_CONFIG[$shard]}
        run_begemot_mapper $shard $input
        answers+=("$OUTPUT/$shard/answers")
    done

    if [[ ${#answers[@]} -gt 0 && -z $SEQUENTIAL ]]; then
        log_step_wrapper "Waiting workers"
        run wait
    fi

    if [[ $test_merger ]]; then
        if [[ -z $only_merger ]]; then
            input="$OUTPUT/Merger/merged_requests"
            merge_answers ${answers[@]} $input
        fi
        local config=${BEGEMOT_CONFIG["Merger"]}
        SEQUENTIAL=true run_begemot_mapper Merger $input
    fi
}

init

progress_step=0
progress_steps=0
INCREMENT_MODE=1 main; main
