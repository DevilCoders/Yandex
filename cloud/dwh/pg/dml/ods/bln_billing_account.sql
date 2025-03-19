DO
$$
    BEGIN
        truncate table ods.bln_billing_account;
        insert into ods.bln_billing_account
        (name, balance, payment_method_id, export_ts, person_id, currency, state, balance_client_id, person_type, ba_id,
         country_code, payment_cycle_type, owner_id, master_account_id, usage_status, type, billing_threshold,
         created_at, balance_contract_id, updated_at, client_id, payment_type, is_isv, is_var, block_reason,
         unblock_reason, paid_at)
        select name,
               balance,
               payment_method_id,
               export_ts,
               person_id,
               currency,
               state,
               balance_client_id,
               person_type,
               ba_id,
               country_code,
               payment_cycle_type,
               owner_id,
               master_account_id,
               usage_status,
               type,
               billing_threshold,
               created_at,
               balance_contract_id,
               updated_at,
               client_id,
               payment_type,
               is_isv,
               is_var,
               block_reason,
               unblock_reason,
               paid_at
        from ods.stg_bln_billing_account;
    END
$$;
