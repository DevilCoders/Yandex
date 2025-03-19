sqlserver-stopped:
    mdb_windows.service_stopped:
        - service_name: "MSSQLSERVER"
        - require_in:
          - test: sqlserver-service-req

