PRAGMA Library("datetime.sql");
PRAGMA Library("helpers.sql");

IMPORT `datetime` SYMBOLS $format_msk_date_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_datetime_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_hour_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_month_name_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_quarter_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_half_year_by_timestamp;
IMPORT `datetime` SYMBOLS $format_msk_year_by_timestamp;
IMPORT `helpers` SYMBOLS $get_md5;

$billing_transactions_table = {{input2->table_quote()}};
$destination_path = {{input1->table_quote()}};
$autopayment_description = "Automatic";

-- Нумеруем биллинг транзакции в рамках окна billing_account_id
$transactions_list = (
    SELECT
        billing_account_id,
        transaction_id as billing_transaction_id,
        IF(StartsWith(description, $autopayment_description), 'auto', 'manual') as billing_transaction_type,
        cast(amount as Double) as billing_transaction_amount,
        currency as billing_transaction_currency,
        created_at,
        status,
        ROW_NUMBER() over w as transaction_number
    FROM
        $billing_transactions_table
    WHERE
        status = 'ok' and transaction_type = 'payments'
    WINDOW
        w AS (PARTITION COMPACT BY billing_account_id ORDER BY created_at)
);

  -- Берем первую транзакцию в окне
$result = (
  SELECT
      $get_md5(billing_transaction_id) AS event_id,
      billing_account_id as event_entity_id,
      'billing_account_first_payment' as event_type,
      'billing_account' as event_entity_type,
      'billing' as event_group,
      cast(created_at as UInt64) as event_timestamp,
      billing_account_id,
      billing_transaction_id,
      billing_transaction_type,
      billing_transaction_amount,
      billing_transaction_currency,
      $format_msk_date_by_timestamp(created_at) as msk_event_dt,
      $format_msk_datetime_by_timestamp(created_at) as msk_event_dttm,
      $format_msk_hour_by_timestamp(created_at) as msk_event_hour,
      $format_msk_month_name_by_timestamp(created_at) as msk_event_month_name,
      $format_msk_quarter_by_timestamp(created_at) as msk_event_quarter,
      $format_msk_half_year_by_timestamp(created_at) as msk_event_half_year,
      $format_msk_year_by_timestamp(created_at) as msk_event_year,
  FROM
      $transactions_list
  WHERE
      transaction_number = 1
);


INSERT INTO $destination_path WITH TRUNCATE
  SELECT
    event_timestamp,
    event_id,
    event_type,
    event_entity_id,
    event_entity_type,
    event_group,
    billing_account_id,
    billing_transaction_id,
    billing_transaction_type,
    billing_transaction_amount,
    billing_transaction_currency,
    msk_event_dt,
    msk_event_dttm,
    msk_event_hour,
    msk_event_month_name,
    msk_event_quarter,
    msk_event_half_year,
    msk_event_year,
  FROM
    $result as r
  ORDER BY
    event_type, event_timestamp, event_entity_id;
