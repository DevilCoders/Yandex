CREATE VIEW cloud_analytics.wtf_dims AS
SELECT
    dims as dim
FROM(
    SELECT
        ['segment','ba_person_type','board_segment','potential','service_name','sku_name','block_reason','is_fraud','ba_usage_status','channel','cohort'] as dims
)
ARRAY JOIN dims