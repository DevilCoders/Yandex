$src_table = {{ param["source_table_path"]->quote() }};
$dst_table = {{input1->table_quote()}};


$prod_input = (SELECT * FROM $src_table);

$result = (
    SELECT
        "a6q" || SUBSTRING(`sku_id`, 3)     AS `sku_id`,
        `sku_name`                          AS `sku_name`,
        `is_committed`                      AS `is_committed`,
        `sku_lazy`                          AS `sku_lazy`,
        `service_long_name`                 AS `service_long_name`,
        `service_name`                      AS `service_name`,
        `subservice_name`                   AS `subservice_name`,
        `cores`                             AS `cores`,
        `cores_number`                      AS `cores_number`,
        `ram`                               AS `ram`,
        `ram_number`                        AS `ram_number`,
        `database`                          AS `database`,
        `core_fraction`                     AS `core_fraction`,
        `core_fraction_number`              AS `core_fraction_number`,
        `preemptible`                       AS `preemptible`,
        `platform`                          AS `platform`,
        `service_group`                     AS `service_group`,
        `mk8s_relevant`                     AS `mk8s_relevant`,
        `is_sustainable`                    AS `is_sustainable`
    FROM
        $prod_input
);

INSERT INTO $dst_table WITH TRUNCATE
SELECT * FROM $result;
