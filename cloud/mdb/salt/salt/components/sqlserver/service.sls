sqlserver-service-req:
    test.nop

sqlserver-service-ready:
    test.nop

sqlserver-service:
    mdb_windows.service_running:
        - service_name: "MSSQLSERVER"
        - require:
          - test: sqlserver-service-req
        - require_in:
          - test: sqlserver-service-ready


sqlserver-service-settings:
    mdb_windows.service_settings:
        - service_name: "MSSQLSERVER"
        - start_type: "Automatic"
        - require:
          - mdb_windows: sqlserver-service


sqlserver-wait-started:
    mdb_sqlserver.wait_started:
        - timeout: 120
        - watch:
          - mdb_windows: sqlserver-service
        - require_in:
          - test: sqlserver-service-ready

