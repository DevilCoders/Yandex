================================================
Перенос данных из одной clickhouse-базы в другую
================================================

Создание структуры таблиц в новой базе
--------------------------------------

Выполняется двумя запросами. Сначала из старой базы дампим **create table** выражения
::
  $  for table in host_resources hostusage hostusage_aggregated_15m; do clickhouse-client -q "show create table ${table}" --host man1-8406.search.yandex.net --port 17353 | xargs | awk '{print $0";"}'; done | sed 's/default\.//'
  CREATE TABLE host_resources ( ts Int64, commit Int64, host String, eventDate Date, cpu_power Float32, cpu_cores UInt8 DEFAULT 0) ENGINE = ReplicatedMergeTree('/clickhouse/tables/host_resources', '{replica}', eventDate, (eventDate, ts, host), 8192);
  CREATE TABLE hostusage ( host String, ts Int64, eventDate Date, mem_usage Float32, cpu_usage Float32, hdd_total Float32, hdd_usage Float32, ssd_total Float32, ssd_usage Float32) ENGINE = ReplicatedMergeTree('/clickhouse/tables/hostusage', '{replica}', eventDate, (eventDate, host, ts), 8192);
  CREATE TABLE hostusage_aggregated_15m ( host String, ts Int64, eventDate Date, mem_usage Float32, cpu_usage Float32, cpu_usage_power_units Float32, hdd_total Float32, hdd_usage Float32, ssd_total Float32, ssd_usage Float32) ENGINE = ReplicatedMergeTree('/clickhouse/tables/hostusage_aggregated_15m', '{replica}', eventDate, (eventDate, host, ts), 8192);
  $

Затем прямо в клиенте в новой базе выполняем то, что получили выводом из предыдущей команды.

Перенос данных экспортом в csv
------------------------------

Сначала дампим данные в csv в старой базе:

::
  :) SELECT * from kpi_graphs_data INTO OUTFILE './kpi_graphs_data.csv' FORMAT CSV;

Затем импортним данные из формата csv в новую базу:

::
  $ clickhouse-client --host man-wwtufkajbj4qp4vv.db.yandex.net --port 9000 --database gencfg_graphs --user kimkim --password aevohngaC0fahchaeh3yohp7eic --query "INSERT INTO kpi_graphs_data FORMAT CSV" <kpi_graphs_data.csv
  $

