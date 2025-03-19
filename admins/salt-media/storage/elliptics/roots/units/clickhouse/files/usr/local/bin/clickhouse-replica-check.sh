#!/bin/bash
if [[ "$1" == "human" ]]; then
  columns='*'
  format='format Vertical'
else
  columns='table'
fi

tables_with_lags=$(echo "
SELECT
    $columns
FROM system.replicas
WHERE
       is_readonly
    OR is_session_expired
    OR future_parts > 20
    OR parts_to_check > 10
    OR queue_size > 20
    OR inserts_in_queue > 10
    OR log_max_index - log_pointer > 10
    OR total_replicas < 2
    OR active_replicas < total_replicas
    OR absolute_delay > 10
    $format
" | clickhouse-client 2>&1 )
if [[ -z $tables_with_lags ]]; then
  echo "0;OK"
else
  if [[ "$1" == "human" ]]; then
     echo "$tables_with_lags"
  else
    echo "2;Broken tables: $tables_with_lags" | tr '\n' ','
  fi
fi
