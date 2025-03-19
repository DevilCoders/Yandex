import yt.wrapper as yt
from cloud.analytics.scripts.yt_migrate import Table

TABLE_NAMES = (
    "achieve_goal_in_referral",
    "add_to_list",
    "add_to_nurture",
    "add_to_opportunity",
    "call_webhook",
    "change_data_value",
    "change_nurture_cadence",
    "change_nurture_track",
    "change_revenue_stage",
    "change_revenue_stage_manually",
    "change_score",
    "change_segment",
    "change_status_in_progression",
    "click_email",
    "click_link",
    "click_sales_email",
    "click_shared_link",
    "delete_lead",
    "disqualify_sweepstakes",
    "earn_entry_in_social_app",
    "email_bounced",
    "email_bounced_soft",
    "email_delivered",
    "enter_sweepstakes",
    "fill_out_form",
    "merge_leads",
    "new_lead",
    "open_email",
    "open_sales_email",
    "push_lead_to_marketo",
    "receive_sales_email",
    "received_forward_to_friend_email",
    "refer_to_social_app",
    "remove_from_list",
    "remove_from_opportunity",
    "request_campaign",
    "sales_email_bounced",
    "send_alert",
    "send_email",
    "send_sales_email",
    "sent_forward_to_friend_email",
    "share_content",
    "sign_up_for_referral_offer",
    "unsubscribe_email",
    "update_opportunity",
    "visit_webpage",
    "vote_in_poll",
    "win_sweepstakes",
)


def upgrade():
    with yt.Transaction():
        for table_name in TABLE_NAMES:
            table = Table(name=table_name)
            current_schema = yt.get('{}/@schema'.format(table.table_path))
            current_schema += [
                {'name': 'lead_dwh_id', 'type': 'string'},
            ]
            yt.unmount_table(table.yt_table_path, sync=True)
            yt.alter_table(
                table.yt_table_path,
                schema=current_schema,
            )

    for table_name in TABLE_NAMES:
        table = Table(name=table_name)
        yt.mount_table(table.yt_table_path, sync=True)
