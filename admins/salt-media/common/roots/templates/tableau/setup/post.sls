cleanup-files:
  file.absent:
    - names:
      - /tmp/clickhouse-odbc
      - /tmp/tableau.deb
      - /tmp/tableau
