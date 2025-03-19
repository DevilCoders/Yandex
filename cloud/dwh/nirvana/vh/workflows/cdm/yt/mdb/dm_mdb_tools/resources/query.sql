-- Input
$mdb_folder = {{ param["mdb_folder"] -> quote() }};
$ui_console_folder = {{ param["ui_console_folder"] -> quote() }};
$api_gateway_folder = {{ param["api_gateway_folder"] -> quote() }};
$billing_bindings_table = {{ param["billing_bindings_table"] -> quote() }};
$billing_accounts_table = {{ param["billing_accounts_table"] -> quote() }};
$crm_tags_table = {{ param["crm_tags_table"] -> quote() }};
$intermediate_table = {{ param["intermediate_table"] -> quote() }};

-- Output
$destination_table = {{ input1 -> table_quote() }};

$parse = DateTime::Parse("%Y-%m-%d %H:%M:%S");
$format = DateTime::Format("%Y-%m-%d");

$days_window = 365;
$start_date = EvaluateExpr(CAST(CurrentUtcDate() - $days_window * Interval("P1D") AS String));
$end_date = EvaluateExpr(CAST(CurrentUtcDate() AS String));

$server_side = (
    SELECT
        cloud_id,
        cluster_id,
        endpoint,
        folder_id,
        request_id,
        user_id,
        user_type,
        $format($parse(iso_eventtime)) AS dt
    FROM RANGE($mdb_folder, $start_date, $end_date)
);

$client_side = (
    SELECT
        request_id,
        String::SplitToList(String::ReplaceAll(url, 'root/', ''), '/')[3] AS service_method,
        String::ReplaceAll(String::ReplaceAll(String::SplitToList(String::ReplaceAll(url, 'root/', ''), '/')[2], 'managed', ''),'-', '') AS service_name,
        'ui console' AS service_provider
    FROM RANGE($ui_console_folder, $start_date, $end_date)
    WHERE String::SplitToList(url, '/')[2] NOT IN ('exchanger','mdb')
    
    UNION ALL
    
    SELECT
        request_id,
        String::AsciiToLower(grpc_method) || String::ReplaceAll(ListLast(String::SplitToList(grpc_service, '.')), 'Service','') AS service_method,
        CASE
            WHEN grpc_service LIKE 'yandex.cloud.dataproc%' THEN 'dataproc'
            ELSE String::SplitToList(grpc_service, '.')[3]
        END AS service_name,
        CASE
            WHEN NOT UserAgent::Parse(user_agent).isBrowser THEN String::SplitToList(String::SplitToList(user_agent, ' ')[0], '/')[0]
            ELSE UserAgent::Parse(user_agent).BrowserName
        END AS service_provider
    FROM
        RANGE($api_gateway_folder, $start_date, $end_date)
    WHERE
        grpc_service LIKE 'yandex.cloud.mdb.%'
        OR grpc_service LIKE 'yandex.cloud.dataproc.%'
);

$mdb_tools = (
    SELECT
        t.service_name                  AS service_name,
        t.service_method                AS service_method,
        t.service_provider              AS service_provider,
        COUNT(DISTINCT t.request_id)    AS request_cnt,
        t.cloud_id                      AS cloud_id,
        t.user_type                     AS user_type,
        t.user_id                       AS user_id,
        t.dt                            AS dt
    FROM (
        SELECT
            client_side.service_name        AS service_name,
            client_side.service_method      AS service_method,
            client_side.service_provider    AS service_provider,
            client_side.request_id          AS request_id,
            server_side.dt                  AS dt,
            MAX(server_side.user_id)        AS user_id,
            MAX(server_side.user_type)      AS user_type,
            MAX(server_side.cloud_id)       AS cloud_id,
            MAX(server_side.endpoint)       AS endpoint,
            MAX(server_side.folder_id)      AS folder_id,
            MAX(server_side.cluster_id)     AS cluster_id
        FROM $server_side AS server_side
            INNER JOIN $client_side AS client_side
                ON client_side.request_id = server_side.request_id
        WHERE
            client_side.service_provider NOT IN ('dataproc-agent')
        GROUP BY
            client_side.service_name,
            client_side.service_method,
            client_side.service_provider,
            client_side.request_id,
            server_side.dt
    ) AS t
    WHERE
        t.cloud_id IS NOT NULL
        AND t.service_name IN ('kafka', 'redis', 'clickhouse', 'postgresql', 'dataproc', 'mysql', 'sqlserver', 'greenplum', 'elasticsearch', 'mongodb')
    GROUP BY
        t.service_name,
        t.service_method,
        t.service_provider,
        t.cloud_id,
        t.user_type,
        t.user_id,
        t.dt
);

$billing_bindings = (
    SELECT
        billing_account_id,
        service_instance_id AS cloud_id
    FROM (
        SELECT
            billing_account_id,
            service_instance_id,
            ROW_NUMBER() OVER(PARTITION BY service_instance_id ORDER BY end_time DESC) AS rn
        FROM $billing_bindings_table
        WHERE service_instance_type='cloud'
            AND billing_account_id IS NOT NULL
    ) AS billing_bindings
    WHERE rn = 1
);

$mdb_tools_clients = (
    SELECT
        billing_bindings.billing_account_id AS billing_account_id,
        COALESCE(String::ReplaceAll(crm.crm_account_name, '\"', "\'"), billing_accounts.name) AS billing_account_name,
        billing_accounts.person_type AS billing_account_type,
        mdb_tools.cloud_id AS cloud_id,
        mdb_tools.service_name AS service_name,
        mdb_tools.service_method AS service_method,
        mdb_tools.service_provider AS service_provider,
        mdb_tools.user_id AS user_id,
        mdb_tools.user_type AS user_type,
        mdb_tools.request_cnt AS request_cnt,
        mdb_tools.dt AS dt
    FROM $mdb_tools AS mdb_tools
    LEFT JOIN $billing_bindings AS billing_bindings
        ON mdb_tools.cloud_id = billing_bindings.cloud_id
    LEFT JOIN $billing_accounts_table AS billing_accounts
        ON billing_bindings.billing_account_id = billing_accounts.billing_account_id
    INNER JOIN $crm_tags_table AS crm
        ON billing_bindings.billing_account_id = crm.billing_account_id and crm.date = mdb_tools.dt
);

INSERT INTO $destination_table WITH TRUNCATE
SELECT
    billing_account_id,
    billing_account_name,
    billing_account_type,
    cloud_id,
    service_name,
    service_method,
    service_provider,
    user_id,
    user_type,
    request_cnt,
    dt
FROM $mdb_tools_clients;
