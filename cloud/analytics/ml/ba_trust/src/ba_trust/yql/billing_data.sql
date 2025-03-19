USE hahn;
PRAGMA yt.TmpFolder = '//home/cloud_analytics/ml/ml_model_features/tmp';
PRAGMA yt.Pool = 'cloud_analytics_pool';

INSERT INTO hahn.`home/cloud_analytics/ml/ba_trust/balance_history` WITH TRUNCATE
SELECT
    `transaction_type`,
    `transaction_id`,
    CAST(`amount` AS float) as `amount`,
    `balance_contract_id`,
    `balance_person_id`,
    `billing_account_id`,
    `created_at`,
    `currency`,
    `description`,
    `is_aborted`,
    `is_secure`,
    `modified_at`,
    `operation_id`,
    `passport_uid`,
    `payload_service_force_3ds`,
    `payment_type`,
    `paymethod_id`,
    `request_id`,
    `status`
FROM hahn.`home/cloud-dwh/data/prod/ods/billing/transactions`