-- constants
$RUB = "RUB";
$USD = "USD";
$KZT = "KZT";

$_1_18_decimal_35_15 = Decimal("1.18", 35, 15);
$_1_2_decimal_35_15 = Decimal("1.2", 35, 15);
$_1_12_decimal_35_15 = Decimal("1.12", 35, 15);

-- get VAT
$get_vat_decimal_35_15 = ($amount, $date, $currency) -> (
  CASE
    WHEN $currency = $RUB AND $date < '2019-01-01' THEN $amount / $_1_18_decimal_35_15
    WHEN $currency = $RUB AND $date >= '2019-01-01' THEN $amount / $_1_2_decimal_35_15
    WHEN $currency = $KZT THEN $amount / $_1_12_decimal_35_15
    ELSE $amount
  END
);

EXPORT $RUB, $USD, $KZT, $get_vat_decimal_35_15;
