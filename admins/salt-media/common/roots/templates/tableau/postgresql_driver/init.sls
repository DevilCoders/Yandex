download-pg-driver:
   cmd.run:
     - name: |
         wget https://downloads.tableau.com/drivers/linux/postgresql/postgresql-{{ salt['pillar.get']('pg_driver_version') }}.jar -O \
           /opt/tableau/tableau_driver/jdbc/postgresql-{{ salt['pillar.get']('pg_driver_version') }}.jar
