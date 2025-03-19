create-jdbc-driver-directories:
   file.directory:
     - user: root
     - group: root
     - mode: 755
     - makedirs: True
     - names:
       - /opt/tableau/tableau_driver/jdbc

clickhouse-odbc-build-prereqs:
    cmd.run:
       - name: |
             wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null && \
             update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-7 60 --slave /usr/bin/g++ g++ /usr/bin/g++-7 && \
             update-alternatives --config gcc

clickhouse-driver-clone-sources:
   cmd.run:
       - name: git clone https://github.com/ClickHouse/clickhouse-odbc.git
       - cwd: /tmp

clickhouse-driver-update-modules:
  cmd.run:
       - name: git submodule update --init
       - cwd: /tmp/clickhouse-odbc

clickhouse-driver-create-build-dir:
   file.directory:
     - user: root
     - group: root
     - mode: 755
     - names:
       - /tmp/clickhouse-odbc/build
       - /usr/local/lib64

configure-build-clickhouse-driver:
  cmd.run:
       - name: cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
       - cwd: /tmp/clickhouse-odbc/build

build-clickhouse-driver:
  cmd.run:
       - name: cmake --build . --config RelWithDebInfo -j 5
       - cwd: /tmp/clickhouse-odbc/build
