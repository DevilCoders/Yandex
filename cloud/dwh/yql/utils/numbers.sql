--constants
$DECIMAL_35_15_INF = CAST('+inf' AS Decimal(35, 15));
$DECIMAL_35_15_NEG_INF = CAST('-inf' AS Decimal(35, 15));

$DECIMAL_35_9_INF = CAST('+inf' AS Decimal(35, 9));
$DECIMAL_35_9_NEG_INF = CAST('-inf' AS Decimal(35, 9));

$DECIMAL_35_15_EPS = Decimal('0.000000000000001', 35, 15);
$DECIMAL_35_9_EPS = Decimal('0.000000001', 35, 9);

-- cast as Decimal from Double
$_double_to_decimal = ($amount, $func) -> ($func(CAST($amount AS String)));

-- cast as Decimal(35,22)
$to_decimal_35_15 = ($amount) -> (CAST($amount AS Decimal(35,15)));
$double_to_decimal_35_15 = ($amount) -> ($_double_to_decimal($amount, $to_decimal_35_15));

-- cast as Decimal(35,9)
$to_decimal_35_9 = ($amount) -> (CAST($amount AS Decimal(35,9)));
$double_to_decimal_35_9 = ($amount) -> ($_double_to_decimal($amount, $to_decimal_35_9));

-- cast as Decimal(22,9)
$to_decimal_22_9 = ($amount) -> (CAST($amount AS Decimal(22,9)));
$double_to_decimal_22_9 = ($amount) -> ($_double_to_decimal($amount, $to_decimal_22_9));

$to_double = ($value) -> (CAST($value as Double));

EXPORT $DECIMAL_35_9_INF, $DECIMAL_35_9_NEG_INF, $DECIMAL_35_15_INF, $DECIMAL_35_15_NEG_INF,
 $DECIMAL_35_15_EPS, $DECIMAL_35_9_EPS, $to_decimal_35_15, $double_to_decimal_35_15,
  $to_decimal_35_9, $double_to_decimal_35_9,
  $to_decimal_22_9, $double_to_decimal_22_9, $to_double;
