CREATE TABLE %(table_name)s
(
	billing_record_total_rub Float64,
	billing_record_committed_use_discount_credit_charge_rub Float64,
	billing_record_committed_use_discount_credit_charge Float64,
	billing_record_disabled_credit_charge_rub Float64,
	billing_record_service_credit_charge_rub Float64,
	billing_record_service_credit_charge Float64,
	billing_record_volume_incentive_credit_charge_rub Float64,
	billing_account_is_verified UInt8,
	billing_account_usage_status_actual String,
	billing_record_disabled_credit_charge Float64,
	sku_name String,
	product_name String,
	billing_record_cost_rub Float64,
	billing_record_total Float64,
	billing_record_pricing_quantity Float64,
	billing_record_cost Float64,
	billing_record_volume_incentive_credit_charge Float64,
	billing_record_date Date,
	billing_record_month String,
	billing_record_trial_credit_charge Float64,
	billing_account_currency String,
	billing_record_credit_rub Float64,
	billing_account_usage_status String,
	billing_record_trial_credit_charge_rub Float64,
	billing_record_monetary_grant_credit_charge_rub Float64,
	billing_record_monetary_grant_credit_charge Float64,
	billing_account_person_type String,
	billing_record_credit Float64,
	service_name String,
	billing_record_end_time DateTime
)
ENGINE = ReplicatedMergeTree('/clickhouse/tables/{shard}/%(table_name)s', '{replica}')
PARTITION BY billing_record_month
ORDER BY (billing_record_date, service_name, sku_name);
