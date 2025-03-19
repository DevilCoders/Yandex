copy-clickhouse-driver:
  cmd.run:
       - name: cp driver/*.so /usr/local/lib64/
       - cwd: /tmp/clickhouse-odbc/build
