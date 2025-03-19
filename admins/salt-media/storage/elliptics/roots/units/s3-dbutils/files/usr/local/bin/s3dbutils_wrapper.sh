{%- set goose_url = salt['pillar.get']('s3-dbutils:postgres:goose_url', '') -%}
{%- set status_dir = salt['pillar.get']('s3-dbutils:dirs:status_dir', '') -%}
{%- set log_dir = salt['pillar.get']('s3-dbutils:dirs:log_dir', '') -%}
{%- set lock_dir = salt['pillar.get']('s3-dbutils:dirs:lock_dir', '') -%}
{%- set rw_user = salt['pillar.get']('s3-dbutils:postgres:rw_user', '') -%}
{%- set ro_user = salt['pillar.get']('s3-dbutils:postgres:ro_user', '') -%}
{%- set mon_user = salt['pillar.get']('s3-dbutils:postgres:mon_user', '') -%}
{%- set pgmeta_conf = salt['pillar.get']('s3-dbutils:postgres:pgmeta:conf_file', '') -%}
{%- set pgpass_file = salt['pillar.get']('s3-dbutils:postgres:pgpass_file', '') -%}
{%- set pg_port = salt['pillar.get']('s3-dbutils:postgres:port', 6432) -%}


{%- set solomon_oauth = pillar['yav']['solomon-s3-dbutils-oauth'] -%}
{%- set solomon_project = salt['pillar.get']('s3-dbutils:solomon:project', 's3-dbutils') -%}
{%- set solomon_service = salt['pillar.get']('s3-dbutils:solomon:service', 'nodes') -%}

{%- set solomon_cluster = salt['pillar.get']('s3-dbutils:solomon:cluster') -%}
{%- set batch_size = salt['pillar.get']('s3-dbutils:scripts:update_chunks_counters:batch_size') -%}  
{%- set threads = salt['pillar.get']('s3-dbutils:scripts:update_chunks_counters_meta:threads') -%}    
{%- set buckets_usage_threads = salt['pillar.get']('s3-dbutils:scripts:update_buckets_usage:threads') -%}    
{%- set queue_warn = salt['pillar.get']('s3-dbutils:monrun:pg_counters_queue:warn') -%}    
{%- set queue_crit = salt['pillar.get']('s3-dbutils:monrun:pg_counters_queue:crit') -%}    
{%- set update_warn = salt['pillar.get']('s3-dbutils:monrun:pg_counters_update:warn') -%}    
{%- set update_crit = salt['pillar.get']('s3-dbutils:monrun:pg_counters_update:crit') -%}    
{%- set chunk_threshold = salt['pillar.get']('s3-dbutils:monrun:pg_counters_queue:chunk_size_threshold') -%} 
{%- set default_script_timeout = salt['pillar.get']('s3-dbutils:scripts:default_timeout') -%}
{%- set critical_errors_filename = salt['pillar.get']('s3-dbutils:scripts:check_chunks_counters:critical_errors_filename') -%} 
{%- set critical_errors_filename_meta = salt['pillar.get']('s3-dbutils:scripts:check_chunks_counters_meta:critical_errors_filename') -%} 
{%- set exclude_bids = salt['pillar.get']('s3-dbutils:scripts:chunk_merger:exclude_bids') -%}

#!/bin/bash

func_name=$1

pgpass="PGPASSFILE={{ pgpass_file }}"

_get_shards() {
    dbtype=$1 # meta or db
    curl {{ goose_url }} 2>/dev/null | jq ".|map(select(.cluster_name==\"${dbtype}\"))|.[0].clusters[]" -c
}

get_meta_shards() { 
    _get_shards "meta"
}

get_s3db_shards() {
    _get_shards "db"
}

get_all_shards() {
  curl {{ goose_url }} 2>/dev/null | jq ".[].clusters[].nodes|map(select(.is_master))[]" -c
}

push-to-solomon() {
    shard=$1
    value=$2
    name=$3
    ts=`date +%s`
    host=`/bin/hostname -f`
    curl "https://solomon.yandex.net/api/v2/push?project={{solomon_project}}&cluster={{solomon_cluster}}&service={{solomon_service}}" \
  -X POST \
  -H "Content-Type: application/json" \
  -H "Authorization: OAuth {{solomon_oauth}}" \
  --data "{
    \"commonLabels\": {
      \"host\": \"${host}\"
    },
    \"metrics\": [
      {
        \"labels\": { \"sensor\": \"${name}\", \"pg_shard\": \"${shard}\" },
        \"ts\": ${ts},
        \"value\": ${value:=0} 
      }
    ]
  }"
}

_metric_updated_rows() {
  host=$1
  out="$2"
  script_name=$3
  updated_rows=`echo "$out" | grep -Eo "Totally updated [0-9]* rows" | awk '{sum += $3} END {print sum}'`
  push-to-solomon ${host} ${updated_rows:=0} updated_chunks_counters
}

_metric_execution_time() {
  host=$1
  out="$2"
  script_name=$3
  starttime=`echo "${out}" | head -n 1 | awk '{print $3" "$4}'`
  stoptime=`echo "${out}" | tail -n 1 | awk '{print $3" "$4}'`
  startms=`date -d "${starttime}" +%s%3N`
  stopms=`date -d "${stoptime}" +%s%3N`
  exec_time=`echo "$stopms - $startms" | bc`
  push-to-solomon ${host} ${exec_time:=0} ${script_name}_exec_time_ms
}

_get_pgmeta_connstr() {
  pgmeta_conf=`cat {{ pgmeta_conf }}`
  pgmeta_host=`echo ${pgmeta_conf} | jq -r .postgres.pgmeta.host`
  pgmeta_user=`echo ${pgmeta_conf} | jq -r .postgres.pgmeta.user`
  pgmeta_password=`echo ${pgmeta_conf} | jq -r .postgres.pgmeta.password`
  pgmeta_port=`echo ${pgmeta_conf} | jq -r .postgres.pgmeta.port`
  pgmeta_db=`echo ${pgmeta_conf} | jq -r .postgres.pgmeta.db_name`
  echo "dbname=${pgmeta_db} host=${pgmeta_host} port=${pgmeta_port} user=${pgmeta_user} password=${pgmeta_password}"
}

update_chunks_counters() {
  local shards=`get_s3db_shards`
  local cmd="/usr/local/bin/update_chunks_counters --batch {{batch_size}} -d"
  local metric_func="_metric_updated_rows"
  local lock="flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                ${metric_func} "None"     "None"
}

update_chunks_counters_meta() {
  local shards=`get_meta_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/update_chunks_counters_meta -u {{ ro_user }} --threads={{ threads }} -p \"${pgmeta_connstr}\" -d"
  local success_criteria="Counters updated successfully"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "${success_criteria}" "None"         "None"     "None"
}

update_bucket_stat() {
  local shards=`get_meta_shards`
  local cmd="/usr/local/bin/update_bucket_stat -d"
  local success_criteria="Buckets stats has been refreshed"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "${success_criteria}" "None"         "None"     "None"
}

update_buckets_size() {
  local shards=`get_meta_shards`
  local cmd="/usr/local/bin/update_buckets_size -d"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         "None"     "None"
}

update_buckets_usage() {
  local shards=`get_meta_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/update_buckets_usage -u {{rw_user}} --threads={{ buckets_usage_threads }} -p \"${pgmeta_connstr}\" -d"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         "None"     "None"
}

update_shard_stat() {
  local shards=`get_meta_shards`
  local cmd="/usr/local/bin/update_shard_stat -d"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         "None"     "None"
}

check_chunks_counters() {
  local args=$2
  local shards=`get_s3db_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/check_chunks_counters --critical-errors-filepath {{ log_dir }}/{{ critical_errors_filename }} ${args} -p \"${pgmeta_connstr}\" -d"
  local lock="zk-flock"
  local on_replica="True"
  local timeout=86400
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         ${timeout} ${on_replica}
}

check_chunks_counters_meta() {
  local shards=`get_meta_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/check_chunks_bounds --critical-errors-filepath {{ log_dir }}/{{ critical_errors_filename_meta }} -u {{ rw_user }} -p \"${pgmeta_connstr}\" -d"
  local lock="zk-flock"
  local on_replica="True"
  local timeout=86400
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         ${timeout} ${on_replica}
}

chunk_splitter() {
  local shards=`get_s3db_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/chunk_splitter -u {{ rw_user }} -p \"${pgmeta_connstr}\" -d"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         "None"     "None"
}

chunk_merger() {
  local shards=`get_s3db_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/merge_chunks -u {{ rw_user }} {% if exclude_bids %} --exclude-bids '{{ exclude_bids }}'{% endif %} -p \"${pgmeta_connstr}\" -d"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         "None"     "None"
}

chunk_mover() {
  local args=$2
  local shards=`get_meta_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/chunk_mover -u {{ rw_user }} -p \"${pgmeta_connstr}\" ${args} -d"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         "None"     "None"
}

chunk_purger() {
  local shards=`get_meta_shards`
  local pgmeta_connstr=`_get_pgmeta_connstr`
  local cmd="/usr/local/bin/chunk_purger -u {{ rw_user }} -p \"${pgmeta_connstr}\" -d"
  local lock="zk-flock"
  #_execute_on_shards ${script_name} "${shards}" "${cmd}" ${lock} "${success_criteria}" ${metric_func} ${timeout} ${on_replica}
  _execute_on_shards  ${1}           "${shards}" "${cmd}" ${lock} "None"                "None"         "None"     "None"
}

_extract_shard_node() {
  shard=$1
  on_replica=$2
  if [[ -z ${on_replica} || ${on_replica} == "None" ]]
  then
      echo ${shard} | jq ".nodes|map(select(.is_master))[0]"
  else
      echo ${shard}| jq ".nodes|map(select(.is_master==false))[0]"
  fi
}

_extract_shard_id() {
  shard=$1
  echo ${shard} | jq .cluster_id -r
}

_extract_shard_host() {
  shard=$1
  on_replica=$2
  _extract_shard_node "${shard}" "${on_replica}" | jq .host -r
}

_extract_shard_port() {
  shard=$1
  on_replica=$2
  _extract_shard_node "${shard}" "${on_replica}" | jq .port -r
}

_extract_shard_db() {
  shard=$1
  on_replica=$2
  _extract_shard_node "${shard}" "${on_replica}" | jq .db_name -r
}

_execute_cmd() {
  local script_name=$2
  local shards=$3
  local cmd=$4
  local lock_type=$5
  local success_criteria=$6
  local metric_func=$7
  local timeout=$8
  local on_replica=$9
  local shard=${10}
  if [[ -z ${timeout} || ${timeout} == "None" ]]
  then
      timeout={{ default_script_timeout }}
  fi
  local timeout_cmd="/usr/bin/timeout ${timeout}"
  local shard_id=`_extract_shard_id "${shard}"`
  local shard_host=`_extract_shard_host "${shard}" "${on_replica}"`
  local shard_port=`_extract_shard_port "${shard}" "${on_replica}"`
  local shard_db=`_extract_shard_db "${shard}" "${on_replica}"`
  local shard_connstr="dbname=${shard_db} host=${shard_host} port=${shard_port} user={{ rw_user }}"
  local shard_status_dir={{ status_dir }}/${script_name}
  local status_file=${shard_status_dir}/shard${shard_id}
  local logfile={{ log_dir }}/${script_name}.log
  
  mkdir -p ${shard_status_dir}
  
  execute() {
      PGPASSFILE={{ pgpass_file }} ${timeout_cmd} ${cmd} "${shard_connstr}" 2>&1 | sed "s/^/${shard_host}\t/" | tee -a ${logfile} ; return ${PIPESTATUS[0]}
  } 
  
  if [[ ${lock_type} == 'zk-flock' ]]
  then
      out=`PGPASSFILE={{ pgpass_file }} /usr/bin/zk-flock -x 125 ${script_name}_shard${shard_id} "${timeout_cmd} ${cmd} '${shard_connstr}'" 2>&1 | sed "s/^/${shard_host}\t/" | tee -a ${logfile} ; return ${PIPESTATUS[0]}`
  else
      out=`execute`
  fi

  local rc=$?
  local res='ok'

  if [[ ${rc} -eq 125 ]]
  then
      echo "0;PASSED" > ${status_file}
      return
  fi
  
  if [[ ! -z ${out} ]]
  then
      if [[ ! -z ${success_criteria} && ${success_criteria} != "None" ]]
      then
          res=`echo "${out}" | grep "${success_criteria}"`
      fi
      if [[ ! -z ${metric_func} && ${metric_func} != "None" ]]
      then
          ${metric_func} ${shard_host} "${out}" ${script_name}
      fi
      _metric_execution_time ${shard_host} "${out}" ${script_name}
  fi
  
  if [[ -z $res || $rc != 0 ]]
  then
      if [[ ${rc} -eq 124 ]]
      then
          echo "2;Timeout[${timeout}]" > ${status_file}
      else
          echo "2;Error[$rc]" > ${status_file}
      fi
  else
      echo "0;OK" > ${status_file}
  fi
}

_execute_on_shards() {
  local script_name=$1
  local shards=$2
  local lock_type=$4
  for shard in ${shards}
  do
      local shard_id=`_extract_shard_id "${shard}"`
      /usr/bin/flock -n {{ lock_dir }}/${script_name}_shard${shard_id} /usr/local/bin/s3dbutils_wrapper.sh _execute_cmd "$@" "${shard}"
  done
}

_monrun() {
  local cmd=$1
  local shards=$2
  local on_replica=$3
  local warn=''
  local crit=''
  for shard in ${shards}
  do
    local host=`_extract_shard_host "${shard}" "${on_replica}"`
    local out=`PGPASSFILE=/etc/s3-dbutils/.pgpass $cmd $host`
    case $(echo $out | cut -d";" -f1) in
      1 ) warn="${warn}${host}: $(echo $out | cut -d';' -f2), " ;; 
      2 ) crit="${crit}${host}: $(echo $out | cut -d';' -f2), " ;;
    esac 
  done
  if [[ ! -z $crit ]]
  then
    echo "2;$crit"
  elif [[ ! -z $warn ]]
  then
    echo "1;$warn"
  else
    echo "0;OK"
  fi
}

pg_counters_queue() {
  local cmd="/usr/bin/python /usr/local/bin/pg_counters_queue.py --dbname s3db --crit {{ queue_crit }} --warn {{ queue_warn }} --table chunks_counters_queue --schema s3 --warn-chunk-size {{ chunk_threshold }} --user {{ ro_user }} --port {{ pg_port }} --host"
  local shards=`get_s3db_shards`
  local on_replica="None"
  _monrun "${cmd}" "${shards}" "${on_replica}"
}

pg_counters_update() {
  local cmd="/usr/bin/python /usr/local/bin/pg_counters_update.py --dbname s3meta --crit {{ update_crit }} --warn {{ update_warn }} --user {{ ro_user }} --port {{ pg_port }} --host"
  local shards=`get_meta_shards`
  local on_replica="None"
  _monrun "${cmd}" "${shards}" "${on_replica}"
}

${func_name} "$@"

