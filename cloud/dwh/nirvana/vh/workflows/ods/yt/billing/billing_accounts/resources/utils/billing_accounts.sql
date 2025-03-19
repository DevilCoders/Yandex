$is_suspended_by_antifraud = ($billing_account) -> {
  -- Определеяет является ли данный биллинг аккаунт заблокированным по причине фрода
  $billing_account_fraud_reason = ListConcat($billing_account.fraud_detected_by, ';');
  RETURN CASE
   WHEN -- Если пользователь не находится в заблокированном состоянии
        $billing_account.state != 'suspended' and $billing_account.state != 'inactive'  THEN False
   WHEN -- Если пользователь заблокирован по причине майнинга
        $billing_account.block_reason = 'mining' THEN True
   WHEN -- Если есть вхождения ключевых слов
        $billing_account_fraud_reason  like '%fraud%' or
        $billing_account_fraud_reason  like '%karma%' or
        $billing_account.block_comment like '%фрод%' or
        $billing_account.block_comment like '%fraud%' or
        $billing_account.block_ticket  like '%CLOUDABUSE-%' or
        -- Частные случаи массовых блокировок > 100 аккаунтов
        $billing_account.block_comment = 'https://t.me/c/1203725153/129979' or
        $billing_account.block_ticket  = 'CLOUDBIZ-1923' or
        $billing_account.block_ticket  = 'CLOUDBIZ-2626' or
        $billing_account.block_ticket  = 'CLOUDINC-1461'
        THEN True
   ELSE False END
};
EXPORT  $is_suspended_by_antifraud;
