PRAGMA AnsiInForEmptyOrNullableItemsCollections;
PRAGMA Library('datetime.sql');

IMPORT `datetime` SYMBOLS $get_date_range_inclusive;
IMPORT `datetime` SYMBOLS $format_date;
IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `datetime` SYMBOLS $MSK_TIMEZONE;
IMPORT `datetime` SYMBOLS $format_msk_month_cohort_by_timestamp;

$billing_accounts_table = {{param["billing_accounts_table"]->quote()}};
$billing_accounts_history_table = {{param["billing_accounts_history_table"]->quote()}};
$crm_account_roles_table = {{param["crm_account_roles_table"]->quote()}};
$crm_accounts_table = {{param["crm_accounts_table"]->quote()}};
$crm_billing_accounts_table = {{param["crm_billing_accounts_table"]->quote()}};
$crm_segments_table = {{param["crm_segments_table"]->quote()}};
$crm_tag_bean_relations_table = {{param["crm_tag_bean_relations_table"]->quote()}};
$crm_tags_table = {{param["crm_tags_table"]->quote()}};
$crm_dimension_bean_relations_table = {{param["crm_dimension_bean_relations_table"]->quote()}};
$crm_dimensions_table = {{param["crm_dimensions_table"]->quote()}};
$crm_users_table = {{param["crm_users_table"]->quote()}};
$destination_path = {{input1->table_quote()}};

$msk_first_dt = AddTimezone(DateTime::MakeDatetime(DateTime::ParseIso8601("2018-01-01T00:00:00+0300")), $MSK_TIMEZONE);
$msk_first_dt_str = $format_date($msk_first_dt);
$msk_last_dt = AddTimezone(CurrentUtcTimestamp(), $MSK_TIMEZONE);
$msk_last_dt_str = $format_date($msk_last_dt);

$roles_statuses = ('confirmed', 'pending_recycling', 'recycled');

$make_empty_str_null = ($str) -> (IF(LENGTH($str) > 0, $str, null));
$crm_no_data_or_unknown = ($no_data, $crm_account_id, $usage_status, $segment) -> (IF($crm_account_id IS NULL and $usage_status != 'service' and $segment !='Mass', 'UNKNOWN', $no_data));

$crm_account_billing_accounts = (
  SELECT
    crm_accounts.crm_account_id AS crm_account_id,
    crm_accounts.crm_account_name AS crm_account_name,
    crm_accounts.industry AS crm_industry,
    crm_accounts.sub_industry AS crm_sub_industry,
    ba.billing_account_id AS billing_account_id,
  FROM $crm_accounts_table AS crm_accounts
  JOIN $crm_billing_accounts_table AS ba USING (crm_account_id)
  WHERE ba.billing_account_id IS NOT NULL AND NOT ba.deleted AND NOT crm_accounts.deleted
);

$billing_accounts = (
  SELECT
    *
  FROM (
    SELECT
      ba.billing_account_id                                AS billing_account_id,
      ba.name                                              AS billing_account_name,
      ba.type                                              AS billing_account_type,
      master_ba.billing_account_id                         AS billing_master_account_id,
      master_ba.name                                       AS billing_master_account_name,
      $format_msk_month_cohort_by_timestamp(ba.created_at) AS billing_account_month_cohort,
      crm_ba.crm_account_id                                AS crm_account_id,
      crm_ba.crm_account_name                              AS crm_account_name,
      crm_ba.crm_industry                                  AS crm_industry,
      crm_ba.crm_sub_industry                              AS crm_sub_industry,
      person_type,
      state,
      usage_status,
      is_fraud,
      is_suspended_by_antifraud,
      payment_type,
      is_isv,
      is_var,
      is_subaccount,
      currency,
      country_code,
      $get_date_range_inclusive(
        $format_msk_date_by_timestamp(created_at),
        $msk_last_dt_str
      ) AS `date`
    FROM $billing_accounts_table AS ba
    LEFT JOIN (SELECT name, billing_account_id FROM $billing_accounts_table) AS master_ba
        ON (ba.master_account_id = master_ba.billing_account_id)
    LEFT JOIN $crm_account_billing_accounts AS crm_ba
        ON (ba.billing_account_id = crm_ba.billing_account_id)
  )
  FLATTEN BY `date`
);

$billing_accounts_hist = (
  SELECT
    billing_account_id                            AS billing_account_id,
    `date`                                        AS `date`,
    MAX_BY(type, updated_at)                      AS type,
    MAX_BY(person_type, updated_at)               AS person_type,
    MAX_BY(is_suspended_by_antifraud, updated_at) AS is_suspended_by_antifraud,
    MAX_BY(usage_status, updated_at)              AS usage_status,
    MAX_BY(state, updated_at)                     AS state,
    MAX_BY(is_fraud, updated_at)                  AS is_fraud,
    MAX_BY(payment_type, updated_at)              AS payment_type,
    MAX_BY(is_isv, updated_at)                    AS is_isv,
    MAX_BY(is_var, updated_at)                    AS is_var,
  FROM
    $billing_accounts_history_table AS ba
  GROUP BY ba.billing_account_id AS billing_account_id, $format_msk_date_by_timestamp(updated_at) AS `date`
);

$billing_accounts_segment_hist = (
  SELECT
    billing_account_id,
    `date`,
    MAX_BY(segment, date_from) AS segment,
  FROM (
    SELECT
      crm_ba.billing_account_id AS billing_account_id,
      segment.crm_segment_name AS segment,
      segment.date_from AS date_from,
      $get_date_range_inclusive(
        NVL($format_msk_date_by_timestamp(segment.date_from), $msk_first_dt_str),
        NVL($format_msk_date_by_timestamp(segment.date_to), $msk_last_dt_str)
      ) AS dates
    FROM $crm_account_billing_accounts AS crm_ba
    JOIN $crm_segments_table AS segment USING (crm_account_id)
    WHERE NOT segment.deleted AND (segment.date_from IS NULL OR segment.date_to IS NULL OR segment.date_from != segment.date_to)
  )
  FLATTEN BY dates AS `date`
  GROUP BY billing_account_id, `date`
);

$crm_billing_account_tags = (
  SELECT
    billing_account_id,
    $make_empty_str_null(String::JoinFromList(ListSort(AGGREGATE_LIST_DISTINCT(tag_id || '&' || tag_name)), ';')) AS tag
  FROM (
    SELECT
      crm_ba.billing_account_id AS billing_account_id,
      crm_tags.crm_tag_id AS tag_id,
      crm_tags.crm_tag_name AS tag_name
    FROM $crm_account_billing_accounts AS crm_ba
    JOIN $crm_tag_bean_relations_table AS crm_tag_bean_rel ON crm_ba.crm_account_id = crm_tag_bean_rel.crm_bean_id
    JOIN $crm_tags_table AS crm_tags ON crm_tag_bean_rel.crm_tag_id = crm_tags.crm_tag_id
    WHERE NOT crm_tag_bean_rel.deleted AND NOT crm_tags.deleted
  )
  GROUP BY billing_account_id
);

$crm_billing_account_dimensions = (
  SELECT
    billing_account_id,
    $make_empty_str_null(String::JoinFromList(ListSort(AGGREGATE_LIST_DISTINCT(dimension_name)), ';')) AS dimension
  FROM (
    SELECT
      crm_ba.billing_account_id AS billing_account_id,
      crm_dimensions.crm_dimension_id AS dimension_id,
      crm_dimensions.crm_dimension_name AS dimension_name
    FROM $crm_account_billing_accounts AS crm_ba
    JOIN $crm_dimension_bean_relations_table AS crm_dimension_bean_rel ON crm_ba.crm_account_id = crm_dimension_bean_rel.crm_bean_id
    JOIN $crm_dimensions_table AS crm_dimensions ON crm_dimension_bean_rel.crm_dimension_id = crm_dimensions.crm_dimension_id
    WHERE NOT crm_dimension_bean_rel.deleted AND NOT crm_dimensions.deleted
  )
  GROUP BY billing_account_id
);

$crm_billing_account_roles_hist = (
  SELECT
    billing_account_id,
    role,
    `date`,
    $make_empty_str_null(String::JoinFromList(ListSort(AGGREGATE_LIST_DISTINCT(user_name)), ',')) AS user_name_grouped,
    MAX_BY(user_name, date_from) AS user_name_max,
  FROM (
    SELECT
      crm_ba.billing_account_id AS billing_account_id,
      users.crm_user_name AS user_name,
      roles.crm_role_name AS role,
      roles.date_from AS date_from,
      $get_date_range_inclusive(
        NVL($format_msk_date_by_timestamp(roles.date_from), $msk_first_dt_str),
        NVL($format_msk_date_by_timestamp(roles.date_to), $msk_last_dt_str)
      ) AS `date`
    FROM $crm_account_billing_accounts AS crm_ba
    JOIN $crm_account_roles_table AS roles ON crm_ba.crm_account_id = roles.crm_account_id
    JOIN $crm_users_table AS users ON roles.assigned_user_id = users.crm_user_id
    WHERE NOT roles.deleted
      AND NOT users.deleted
      AND roles.status IN $roles_statuses
      AND (roles.date_from IS NULL OR roles.date_to IS NULL OR roles.date_from != roles.date_to)
  )
  FLATTEN BY `date`
  GROUP BY billing_account_id, `date`, role
);

$crm_billing_account_roles_attrs_hist = (
  SELECT
    billing_account_id,
    `date`,
    MAX(account_owner) AS account_owner,
    MAX(architect) AS architect,
    MAX(bus_dev) AS bus_dev,
    MAX(partner_manager) AS partner_manager,
    MAX(sales) AS sales,
    MAX(tam) AS tam,
  FROM (
    SELECT
      billing_account_id,
      `date`,
      IF(role = 'account_owner', user_name_max, null) AS account_owner,
      IF(role = 'architect', user_name_grouped, null) AS architect,
      IF(role = 'bus_dev', user_name_grouped, null) AS bus_dev,
      IF(role = 'partner_manager', user_name_grouped, null) AS partner_manager,
      IF(role = 'sales', user_name_grouped, null) AS sales,
      IF(role = 'tam', user_name_max, null) AS tam,
    FROM $crm_billing_account_roles_hist
  )
  GROUP BY billing_account_id, `date`
);

$crm_info = (
  SELECT
    NVL(segments.billing_account_id, tags.billing_account_id, dimensions.billing_account_id, roles.billing_account_id) AS billing_account_id,
    NVL(segments.`date`, roles.`date`) AS `date`,
    segments.segment AS segment,
    tags.tag AS tag,
    dimension AS dimension,
    roles.account_owner AS account_owner,
    roles.architect AS architect,
    roles.bus_dev AS bus_dev,
    roles.partner_manager AS partner_manager,
    roles.tam AS tam,
    roles.sales AS sales,
  FROM $billing_accounts_segment_hist AS segments
  FULL JOIN $crm_billing_account_tags AS tags ON (segments.billing_account_id = tags.billing_account_id)
  FULL JOIN $crm_billing_account_dimensions AS dimensions ON (segments.billing_account_id = dimensions.billing_account_id)
  FULL JOIN $crm_billing_account_roles_attrs_hist AS roles ON (segments.billing_account_id = roles.billing_account_id AND segments.`date` = roles.`date`)
);

$result = (
  SELECT
    ba.`date`                                                                                                                       AS `date`,
    ba.billing_account_id                                                                                                           AS billing_account_id,
    ba.billing_account_name                                                                                                         AS billing_account_name,
    ba.billing_account_month_cohort                                                                                                 AS billing_account_month_cohort,
    ba.currency                                                                                                                     AS currency,
    ba.is_subaccount                                                                                                                AS is_subaccount,
    ba.country_code                                                                                                                 AS country_code,
    ba.crm_account_id                                                                                                               AS crm_account_id,
    ba.crm_account_name                                                                                                             AS crm_account_name,
    ba.crm_industry                                                                                                                 AS crm_industry,
    ba.crm_sub_industry                                                                                                             AS crm_sub_industry,
    IF(ba.crm_account_id is null, 'ba_' || ba.billing_account_id, 'crm_acc_' || ba.crm_account_id)                                  AS client_id,
    ba.billing_master_account_id                                                                                                    AS billing_master_account_id,
    ba.billing_master_account_name                                                                                                  AS billing_master_account_name,
    ba.billing_account_type                                                                                                         AS billing_account_type_current,
    LAST_VALUE(ba_hist.type) IGNORE NULLS OVER w_to_current_row                                                                     AS billing_account_type,
    ba.person_type                                                                                                                  AS person_type_current,
    LAST_VALUE(ba_hist.person_type) IGNORE NULLS OVER w_to_current_row                                                              AS person_type,
    ba.is_suspended_by_antifraud                                                                                                    AS is_suspended_by_antifraud_current,
    LAST_VALUE(ba_hist.is_suspended_by_antifraud) IGNORE NULLS OVER w_to_current_row                                                AS is_suspended_by_antifraud,
    ba.usage_status                                                                                                                 AS usage_status_current,
    LAST_VALUE(ba_hist.usage_status) IGNORE NULLS OVER w_to_current_row                                                             AS usage_status,
    ba.state                                                                                                                        AS state_current,
    LAST_VALUE(ba_hist.state) IGNORE NULLS OVER w_to_current_row                                                                    AS state,
    ba.is_fraud                                                                                                                     AS is_fraud_current,
    LAST_VALUE(ba_hist.is_fraud) IGNORE NULLS OVER w_to_current_row                                                                 AS is_fraud,
    ba.payment_type                                                                                                                 AS payment_type_current,
    LAST_VALUE(ba_hist.payment_type) IGNORE NULLS OVER w_to_current_row                                                             AS payment_type,
    ba.is_isv                                                                                                                       AS is_isv_current,
    LAST_VALUE(ba_hist.is_isv) IGNORE NULLS OVER w_to_current_row                                                                   AS is_isv,
    ba.is_var                                                                                                                       AS is_var_current,
    LAST_VALUE(ba_hist.is_var) IGNORE NULLS OVER w_to_current_row                                                                   AS is_var,
    NVL(LAST_VALUE(crm.segment) IGNORE NULLS OVER w_global, LAST_VALUE(IF(ba.usage_status = 'service', 'Cloud Service', 'Mass')) OVER w_global)
                                                                                                                                    AS segment_current,
    NVL(LAST_VALUE(crm.segment) IGNORE NULLS OVER w_to_current_row, IF(LAST_VALUE(ba_hist.usage_status) IGNORE NULLS OVER w_to_current_row = 'service', 'Cloud Service', 'Mass'))
                                                                                                                                    AS segment,
    --
    LAST_VALUE(crm.tag) IGNORE NULLS OVER w_global                                                                                  AS crm_account_tags,
    LAST_VALUE(crm.dimension) IGNORE NULLS OVER w_global                                                                            AS crm_account_dimensions,
    LAST_VALUE(crm.account_owner) OVER w_global                                                                                     AS account_owner_current,
    crm.account_owner                                                                                                               AS account_owner,
    LAST_VALUE(crm.architect) OVER w_global                                                                                         AS architect_current,
    crm.architect                                                                                                                   AS architect,
    LAST_VALUE(crm.bus_dev) OVER w_global                                                                                           AS bus_dev_current,
    crm.bus_dev                                                                                                                     AS bus_dev,
    LAST_VALUE(crm.partner_manager) OVER w_global                                                                                   AS partner_manager_current,
    crm.partner_manager                                                                                                             AS partner_manager,
    LAST_VALUE(crm.tam) OVER w_global                                                                                               AS tam_current,
    crm.tam                                                                                                                         AS tam,
    LAST_VALUE(crm.sales) OVER w_global                                                                                             AS sales_current,
    crm.sales                                                                                                                       AS sales,
    LAST_VALUE(ba.crm_account_id) IGNORE NULLS OVER w_global                                                                        AS crm_account_id_current
  FROM $billing_accounts AS ba
  LEFT JOIN $billing_accounts_hist AS ba_hist ON (ba.billing_account_id = ba_hist.billing_account_id AND ba.`date` = ba_hist.`date`)
  LEFT JOIN $crm_info AS crm ON (ba.billing_account_id = crm.billing_account_id AND ba.`date` = crm.`date`)
  WINDOW
    w_to_current_row AS (PARTITION BY ba.billing_account_id ORDER BY ba.`date` ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW),
    w_global         AS (PARTITION BY ba.billing_account_id ORDER BY ba.`date` ROWS BETWEEN UNBOUNDED PRECEDING AND UNBOUNDED FOLLOWING)
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  `date`                                                                                                                                    AS `date`,
  billing_account_id                                                                                                                        AS billing_account_id,
  billing_account_name                                                                                                                      AS billing_account_name,
  billing_account_month_cohort                                                                                                              AS billing_account_month_cohort,
  currency                                                                                                                                  AS currency,
  is_subaccount                                                                                                                             AS is_subaccount,
  country_code                                                                                                                              AS country_code,
  crm_account_id                                                                                                                            AS crm_account_id,
  crm_account_name                                                                                                                          AS crm_account_name,
  client_id                                                                                                                                 AS client_id,
  billing_master_account_id                                                                                                                 AS billing_master_account_id,
  billing_master_account_name                                                                                                               AS billing_master_account_name,
  billing_account_type_current                                                                                                              AS billing_account_type_current,
  billing_account_type                                                                                                                      AS billing_account_type,
  person_type_current                                                                                                                       AS person_type_current,
  person_type                                                                                                                               AS person_type,
  is_suspended_by_antifraud_current                                                                                                         AS is_suspended_by_antifraud_current,
  is_suspended_by_antifraud                                                                                                                 AS is_suspended_by_antifraud,
  usage_status_current                                                                                                                      AS usage_status_current,
  usage_status                                                                                                                              AS usage_status,
  state_current                                                                                                                             AS state_current,
  state                                                                                                                                     AS state,
  is_fraud_current                                                                                                                          AS is_fraud_current,
  is_fraud                                                                                                                                  AS is_fraud,
  payment_type_current                                                                                                                      AS payment_type_current,
  payment_type                                                                                                                              AS payment_type,
  is_isv_current                                                                                                                            AS is_isv_current,
  is_isv                                                                                                                                    AS is_isv,
  is_var_current                                                                                                                            AS is_var_current,
  is_var                                                                                                                                    AS is_var,
  segment_current                                                                                                                           AS segment_current,
  segment                                                                                                                                   AS segment,
  --
  NVL(crm_account_tags        , 'No Account Tag')                                                                                           AS crm_account_tags,
  NVL(crm_account_dimensions  , 'No Account Dimension')                                                                                     AS crm_account_dimensions,
  NVL(crm_industry            , $crm_no_data_or_unknown('No Industry'                   ,crm_account_id ,usage_status,segment))             AS crm_industry,
  NVL(crm_sub_industry        , $crm_no_data_or_unknown('No Sub Industry'               ,crm_account_id ,usage_status,segment))             AS crm_sub_industry,
  NVL(account_owner_current   , $crm_no_data_or_unknown('No Account Owner'              ,crm_account_id_current   ,usage_status,segment))   AS account_owner_current,
  NVL(account_owner           , $crm_no_data_or_unknown('No Account Owner'              ,crm_account_id ,usage_status,segment))             AS account_owner,
  NVL(architect_current       , $crm_no_data_or_unknown('No Architect'                  ,crm_account_id_current   ,usage_status,segment))   AS architect_current,
  NVL(architect               , $crm_no_data_or_unknown('No Architect'                  ,crm_account_id ,usage_status,segment))             AS architect,
  NVL(bus_dev_current         , $crm_no_data_or_unknown('No BizDev'                     ,crm_account_id_current   ,usage_status,segment))   AS bus_dev_current,
  NVL(bus_dev                 , $crm_no_data_or_unknown('No BizDev'                     ,crm_account_id ,usage_status,segment))             AS bus_dev,
  NVL(partner_manager_current , $crm_no_data_or_unknown('No Partner Manager'            ,crm_account_id_current   ,usage_status,segment))   AS partner_manager_current,
  NVL(partner_manager         , $crm_no_data_or_unknown('No Partner Manager'            ,crm_account_id ,usage_status,segment))             AS partner_manager,
  NVL(tam_current             , $crm_no_data_or_unknown('No Technical Account Manager'  ,crm_account_id_current   ,usage_status,segment))   AS tam_current,
  NVL(tam                     , $crm_no_data_or_unknown('No Technical Account Manager'  ,crm_account_id ,usage_status,segment))             AS tam,
  NVL(sales_current           , $crm_no_data_or_unknown('No Sales Manager'              ,crm_account_id_current   ,usage_status,segment))   AS sales_current,
  NVL(sales                   , $crm_no_data_or_unknown('No Sales Manager'              ,crm_account_id ,usage_status,segment))             AS sales
FROM $result
ORDER BY `date`, `billing_account_id`;
