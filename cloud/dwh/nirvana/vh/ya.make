OWNER(g:cloud-dwh)

PY3_LIBRARY()

PEERDIR(
    nirvana/valhalla/src
    cloud/dwh/nirvana/config
    cloud/dwh/nirvana/vh/common
    cloud/dwh/nirvana/vh/config

#   Workflows
#   CDM
    cloud/dwh/nirvana/vh/workflows/cdm/yt/antifraud/dm_ba_af_falsepositive
    cloud/dwh/nirvana/vh/workflows/cdm/yt/console/dm_console_automatic_clicks_events
    cloud/dwh/nirvana/vh/workflows/cdm/yt/console/dm_console_user_events
    cloud/dwh/nirvana/vh/workflows/cdm/yt/datalens/dm_datalens_funnel_with_promo
    cloud/dwh/nirvana/vh/workflows/cdm/yt/datalens/dm_datalens_funnel_without_promo
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_ba_consumption
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_ba_crm_tags
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_ba_history
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_base_consumption
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_billing_record_user_labels
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_crm_calls
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_crm_leads
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_events
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_folder_consumption
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_quota_usage_and_limits
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_sku_actual_prices
    cloud/dwh/nirvana/vh/workflows/cdm/yt/dm_yc_consumption
    cloud/dwh/nirvana/vh/workflows/cdm/yt/iaas/dm_iaas_consumption
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_consumption_with_marketing_attribution
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_attribution
    cloud/dwh/nirvana/vh/workflows/cdm/yt/marketing/dm_marketing_events
    cloud/dwh/nirvana/vh/workflows/cdm/yt/mdb/dm_mdb_tools
    cloud/dwh/nirvana/vh/workflows/cdm/yt/staff/dm_cloud_department_employees
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/dm_yc_support_comment_slas
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/dm_yc_support_component_issues
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/dm_yc_support_issue_summons
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/dm_yc_support_issues
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/dm_yc_support_tag_issues
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/line_two/dm_first_response_timer_sla
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/line_two/dm_yc_support_comment_slas
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/line_two/dm_yc_support_component_issues
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/line_two/dm_yc_support_issues
    cloud/dwh/nirvana/vh/workflows/cdm/yt/support/line_two/dm_yc_support_tag_issues
#   ODS
    cloud/dwh/nirvana/vh/workflows/ods/yt/api_gateway/api_gateway_event
    cloud/dwh/nirvana/vh/workflows/ods/yt/backoffice/applications
    cloud/dwh/nirvana/vh/workflows/ods/yt/backoffice/events
    cloud/dwh/nirvana/vh/workflows/ods/yt/backoffice/events_services
    cloud/dwh/nirvana/vh/workflows/ods/yt/backoffice/participants
    cloud/dwh/nirvana/vh/workflows/ods/yt/backoffice/services
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/balance_reports
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts_balance
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_accounts_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_records
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/billing_metrics
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/budgeter_logs
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/budgets
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/committed_use_discount_templates
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/committed_use_discounts
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/conversion_rates
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/grant_policies
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/labels_maps
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/monetary_grant_counters
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/monetary_grant_offers
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/monetary_grants
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/operation_objects
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/operations
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/person_data
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/pricing_versions
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/publisher_accounts
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/referral_code_usage
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/referrer_reports
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/referrer_reward_balance_withdrawals
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/revenue_reports
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/schemas
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/service_instance_bindings
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/services
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/sku_labels
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/sku_overrides
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/sku_reports
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/skus
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/subscription_to_committed_use_discount
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/subscriptions
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/support_subscriptions
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/support_templates
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/tiered_sku_counters
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/transactions
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/var_adjustments
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/var_incentives
    cloud/dwh/nirvana/vh/workflows/ods/yt/billing/volume_incentives
    cloud/dwh/nirvana/vh/workflows/ods/yt/cloud_analytics/sku_tags
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/cpu_usage_metrics
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/default_quotas
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/quota_limits
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute/quota_usage
    cloud/dwh/nirvana/vh/workflows/ods/yt/compute_disks_usage_by_billing
    cloud/dwh/nirvana/vh/workflows/ods/yt/console/ui_console_event
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/billing_accounts
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_account_roles
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_account_roles_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts_contacts
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_accounts_opportunities
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_calls
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_calls_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_contacts
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_contacts_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_dimensions
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_email_address_bean_relations
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_email_addresses
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_leads
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_leads_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_leads_billing_accounts
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_notes
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunities
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunities_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunities_contacts
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunity_resource_lines
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_opportunity_resources
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_plans
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_plans_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_product_categories
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_product_templates
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_product_templates_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_revenue_line_items_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_segments
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_tags
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_tasks
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_tasks_audit
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_team_sets_teams
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/dimension_bean_relations
    cloud/dwh/nirvana/vh/workflows/ods/yt/crm/tag_bean_relations
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/clouds
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/clouds_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/federated_users
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/federated_users_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/federations
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/federations_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/folders
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/folders_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/organizations
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/organizations_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/passport_users
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/passport_users_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/quota_manager/quota_request_limits
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/quota_manager/quota_request_limits_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/quota_manager/quota_requests
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/quota_manager/quota_requests_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/resource_memberships
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/resource_memberships_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/service_accounts
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/service_accounts_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/role_bindings
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/role_bindings_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/subjects
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/subjects_history
    cloud/dwh/nirvana/vh/workflows/ods/yt/iam/users
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/api_request
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/clouds
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/clusters
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/disk_type
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/flavors
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/folders
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/hosts
    cloud/dwh/nirvana/vh/workflows/ods/yt/mdb/subclusters
    cloud/dwh/nirvana/vh/workflows/ods/yt/metrika/hit_log
    cloud/dwh/nirvana/vh/workflows/ods/yt/metrika/visit_log
    cloud/dwh/nirvana/vh/workflows/ods/yt/nbs/kikimr_disk_used_space
    cloud/dwh/nirvana/vh/workflows/ods/yt/nbs/nbs_disk_purchased_space
    cloud/dwh/nirvana/vh/workflows/ods/yt/nbs/nbs_disk_used_space
    cloud/dwh/nirvana/vh/workflows/ods/yt/nbs/nbs_nrd_used_space
    cloud/dwh/nirvana/vh/workflows/ods/yt/solomon/yandexcloud_cpu_usage
    cloud/dwh/nirvana/vh/workflows/ods/yt/solomon/yandexcloud_cpu_utilization
    cloud/dwh/nirvana/vh/workflows/ods/yt/staff/persons
    cloud/dwh/nirvana/vh/workflows/ods/yt/staff/persons_hist
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/comments
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/components
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/issue_components
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/issue_events
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/issue_slas
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/issue_tags
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_line_two/issues
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/comments
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/components
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/issue_components
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/issue_events
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/issue_slas
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/issue_tags
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/cloud_support/issues
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/common/types
    cloud/dwh/nirvana/vh/workflows/ods/yt/startrek/common/users
    cloud/dwh/nirvana/vh/workflows/ods/yt/statbox/currency_rates
    cloud/dwh/nirvana/vh/workflows/ods/yt/support/comments
    cloud/dwh/nirvana/vh/workflows/ods/yt/support/issues
    cloud/dwh/nirvana/vh/workflows/ods/yt/yandex_connect/organizations
    cloud/dwh/nirvana/vh/workflows/ods/yt/yandex_connect/users
#   RAW
    cloud/dwh/nirvana/vh/workflows/raw/yt/cloud_analytics/sku_tags
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/kikimr_disk_used_space
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/nbs_disk_purchased_space
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/nbs_disk_used_space
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/nbs_nrd_used_space
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/yandexcloud_cpu_usage
    cloud/dwh/nirvana/vh/workflows/raw/yt/solomon/yandexcloud_cpu_utilization
    cloud/dwh/nirvana/vh/workflows/raw/yt/staff/persons
#   STG
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/ba_crm_tags
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/became_trial
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/common
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/feature_flag_changed
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_paid_consumption
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_payment
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_service_consumption
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/first_trial_consumption
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/billing/state_changed
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/iam/cloud_created
    cloud/dwh/nirvana/vh/workflows/stg/yt/cdm/events/iam/create_organization
#   EXPORT
    #    TODO: Actualize
    #    cloud/dwh/nirvana/vh/workflows/cdm/pg/dim_event
    #    cloud/dwh/nirvana/vh/workflows/cdm/pg/dim_lead
    #    cloud/dwh/nirvana/vh/workflows/cdm/yt/fct_client_ba_info
    #    cloud/dwh/nirvana/vh/workflows/cdm/yt/fct_client_cloud_webpage_visit
    #    cloud/dwh/nirvana/vh/workflows/cdm/yt/fct_client_consumption
    #    cloud/dwh/nirvana/vh/workflows/cdm/yt/fct_client_nurture_stream
    #    cloud/dwh/nirvana/vh/workflows/cdm/yt/fct_client_support_request
    #    cloud/dwh/nirvana/vh/workflows/export/pg/crm_form_ba_request
    #    cloud/dwh/nirvana/vh/workflows/export/pg/crm_form_compete_promo
    #    cloud/dwh/nirvana/vh/workflows/export/pg/crm_form_mdb_choose_db_test
    #    cloud/dwh/nirvana/vh/workflows/export/pg/crm_form_promo
    #    cloud/dwh/nirvana/vh/workflows/export/pg/crm_lead_event_attendance
    #    cloud/dwh/nirvana/vh/workflows/export/pg/marketing
    #    cloud/dwh/nirvana/vh/workflows/ods/pg/backoffice
    #    cloud/dwh/nirvana/vh/workflows/ods/pg/billing
    #    cloud/dwh/nirvana/vh/workflows/ods/pg/forms
    #    cloud/dwh/nirvana/vh/workflows/ods/pg/iam
    #    cloud/dwh/nirvana/vh/workflows/ods/pg/site
    #    cloud/dwh/nirvana/vh/workflows/other/yt_cleanup
    #    cloud/dwh/nirvana/vh/workflows/raw/yt/backoffice
    #    cloud/dwh/nirvana/vh/workflows/raw/yt/crm
    #    cloud/dwh/nirvana/vh/workflows/raw/yt/iam/cloud_creator
    #    cloud/dwh/nirvana/vh/workflows/raw/yt/iam/cloud_folder
    #    cloud/dwh/nirvana/vh/workflows/raw/yt/startrek
    cloud/dwh/nirvana/vh/workflows/export/cdm/yt/ch/dm_yc_consumption
    cloud/dwh/nirvana/vh/workflows/export/yandex-analytics/dm_yc_consumption_by_crm_segment
    cloud/dwh/nirvana/vh/workflows/export/yandex-analytics/dm_yc_consumption_by_sku_service_name
)

PY_SRCS(__init__.py)

END()

RECURSE(
    common
    config
    workflows
)
