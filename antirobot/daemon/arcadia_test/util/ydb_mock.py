import ydb


class Ydb:
    def create_table_if_not_exists(self, query):
        # NOTE: во всех Test Suites используется 1 общий инстанс YDB
        try:
            self.session.execute_scheme(query)
        except ydb.issues.GenericError as e:
            if "Table name conflict" not in e.message:
                raise e

    def __init__(self, endpoint, database):
        self.endpoint = endpoint
        self.database = database

        driver = ydb.Driver(ydb.DriverConfig(endpoint, database))
        driver.wait()
        self.session = ydb.retry_operation_sync(lambda: driver.table_client.session().create())
        assert self.session.transaction().execute('select 1 as cnt;', commit_tx=True)[0].rows[0].cnt == 1

        self.create_table_if_not_exists("""
            CREATE TABLE `smart-sessions`
            (
                key UInt64,
                spravka String,
                `timestamp` Timestamp,
                PRIMARY KEY (key)
            ) WITH (
                TTL = Interval("PT1800S") ON `timestamp`
            );
        """)

        self.create_table_if_not_exists("""
            CREATE TABLE `captcha-settings`
            (
                captcha_id String,
                cloud_id   String,
                folder_id  String,
                client_key String,
                server_key String,
                created_at Timestamp,
                updated_at Timestamp,
                data String,

                PRIMARY KEY (captcha_id),
                INDEX cloud_index      GLOBAL ON (cloud_id),
                INDEX folder_index     GLOBAL ON (folder_id),
                INDEX client_key_index GLOBAL ON (client_key),
                INDEX server_key_index GLOBAL ON (server_key)
            );
        """)
        # TODO: наверное нужен индекс (captcha_id, created_at)

        self.create_table_if_not_exists("""
            CREATE TABLE `cloud-quotas`
            (
                cloud_id String,
                metric   String,
                usage    Double,
                lim      Int64,

                PRIMARY KEY (cloud_id, metric)
            );
        """)

        self.create_table_if_not_exists("""
            CREATE TABLE `cloud-operations`
            (
                operation_id String,
                folder_id    String,
                captcha_id   String,
                created_at   Timestamp,
                data         String,

                PRIMARY KEY (operation_id),
                INDEX folder_index GLOBAL ON (folder_id, created_at)
            );
        """)
