PRAGMA File('currency_rates.json', {{ param["raw_currency_rates"]->quote() }});
PRAGMA Library('numbers.sql');
PRAGMA Library('datetime.sql');

IMPORT `datetime` SYMBOLS $get_date_range_inclusive;
IMPORT `datetime` SYMBOLS $format_date;
IMPORT `datetime` SYMBOLS $MSK_TIMEZONE;
IMPORT `numbers` SYMBOLS $double_to_decimal_35_15;

$destination_path = {{input1->table_quote()}};

$currencies = ['USD', 'KZT'];
$msk_first_dt = AddTimezone(DateTime::MakeDatetime(DateTime::ParseIso8601("2018-01-01T00:00:00+0300")), $MSK_TIMEZONE);
$msk_first_dt_str = $format_date($msk_first_dt);
$msk_last_dt = AddTimezone(CurrentUtcTimestamp() + INTERVAL('P7D'), $MSK_TIMEZONE);
$msk_last_dt_str = $format_date($msk_last_dt);

$currency_rates_json = FileContent('currency_rates.json');

$currency_rates_dict = (
  SELECT
    Yson::ConvertTo(Yson::ParseJson($currency_rates_json), Dict<String,Dict<String, Double>>) as daily_info
);

$currency_rates = (
  SELECT
    `date`,
    quotes.0 as currency,
    $double_to_decimal_35_15(quotes.1) as quote
  FROM (
    SELECT
      daily_info.0 as `date`,
      daily_info.1 as quotes
    FROM $currency_rates_dict
    FLATTEN DICT BY daily_info
  )
  FLATTEN DICT BY quotes
);

$dates = (
  SELECT
    `date`,
    currency
  FROM (
    SELECT
      $get_date_range_inclusive($msk_first_dt_str, $msk_last_dt_str) AS `date`,
      $currencies as currency,
  )
  FLATTEN BY (`date`, currency)
);

$result = (
  SELECT
    dates.`date` as `date`,
    dates.currency as currency,
    LAST_VALUE(rates.quote) IGNORE NULLS OVER w_to_current_row as quote,
  FROM $dates as dates
  LEFT JOIN $currency_rates as rates USING(`date`, currency)
  WINDOW w_to_current_row as (PARTITION BY dates.currency ORDER BY dates.`date` ROWS BETWEEN UNBOUNDED PRECEDING AND CURRENT ROW)
);

INSERT INTO $destination_path WITH TRUNCATE
SELECT
  `date`,
  currency,
  quote,
FROM $result
ORDER BY `date`, currency
