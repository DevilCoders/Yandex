import re


def get_counter_ids(service_config, is_captcha_api_service=False):
    service_signals = [
        "robots_with_captcha_deee",
        "unique_random_factors_deee",
        "wizard_errors_deee",
        "whitelist_queries_deee",
        "whitelist_queries_susp_deee",
        "missed_once_robots_deee",
        "missed_multiple_robots_deee",
        "captcha_voice_downloads_deee",
        "captcha_incorrect_inputs_deee",
        "captcha_incorrect_inputs_trusted_users_deee",
        "captcha_incorrect_checkbox_inputs_deee",
        "captcha_incorrect_checkbox_inputs_trusted_users_deee",
        "captcha_preprod_incorrect_inputs_deee",
        "captcha_preprod_incorrect_inputs_trusted_users_deee",
        "captcha_preprod_correct_inputs_deee",
        "captcha_preprod_correct_inputs_trusted_users_deee",
        "captcha_preprod_incorrect_checkbox_inputs_deee",
        "captcha_preprod_incorrect_checkbox_inputs_trusted_users_deee",
        "captcha_preprod_correct_checkbox_inputs_deee",
        "captcha_preprod_correct_checkbox_inputs_trusted_users_deee",
        "captcha_fury_ok_to_fail_changes_deee",
        "captcha_fury_fail_to_ok_changes_deee",
        "captcha_fury_preprod_ok_to_fail_changes_deee",
        "captcha_fury_preprod_fail_to_ok_changes_deee",
        "banned_by_cbb_total_deee",
        "banned_by_cbb_ip_based_deee",
        "banned_by_cbb_regexp_deee",
        "panic_maybanfor_status_ahhh",
        "panic_canshowcaptcha_status_ahhh",
        "panic_morda_status_ahhh",
        "panic_dzensearch_status_ahhh",
        "panic_neverban_status_ahhh",
        "panic_neverblock_status_ahhh",
        "panic_cbb_status_ahhh",
        "panic_preview_ident_type_status_ahhh",
        "server_error_ahhh",
        "many_requests_deee",
        "many_requests_mobile_deee",
        "yandex_requests_deee",
        "crawler_requests_deee",
        "invalid_jws_deee",
        "unknown_valid_jws_deee",
        "android_valid_jws_deee",
        "ios_valid_jws_deee",
        "android_valid_expired_jws_deee",
        "ios_valid_expired_jws_deee",
        "default_jws_deee",
        "default_expired_jws_deee",
        "android_susp_jws_deee",
        "ios_susp_jws_deee",
        "android_susp_expired_jws_deee",
        "ios_susp_expired_jws_deee",
        "invalid_yandex_trust_deee",
        "valid_yandex_trust_deee",
        "valid_expired_yandex_trust_deee",
        "marked_by_yql_deee",
    ]

    service_exp_bin_signals = [
        "ban_source_ip_deee",
        "ban_fw_source_ip_deee",
        "cut_requests_by_cbb_deee",
        "cacher_request_whitelisted_deee",
    ]

    group_signals = [
        "block_responses_user_marked_with_Lcookie_deee",
        "block_responses_with_Lcookie_deee",
    ]

    id_signals = [
        "robot_set_size_ahhh",
    ]

    service_id_signals = [
        "captcha_shows_deee",
        "captcha_advanced_shows_deee",
        "captcha_initial_redirects_deee",
        "captcha_redirects_by_mxnet_deee",
        "captcha_redirects_by_yql_deee",
        "captcha_redirects_by_cbb_deee",
        "captcha_correct_checkbox_inputs_trusted_users_deee",
        "captcha_correct_inputs_trusted_users_deee",
    ]

    service_id_exp_bin_signals = [
        "captcha_correct_checkbox_inputs_deee",
        "captcha_correct_inputs_deee",
        "captcha_image_shows_deee",
        "captcha_redirects_bin_deee",
        "requests_passed_to_service_deee",
        "with_degradation_requests_deee",
        "with_suspiciousness_requests_deee",
        "requests_deee",
        "block_responses_total_deee",
    ]

    id_group_signals = [
        "requests_trusted_users_deee",
        "requests_with_captcha_deee",
        "requests_with_captcha_trusted_users_deee",
        "robots_deee",
        "captcha_redirects_trusted_users_deee",
        "captcha_advanced_redirects_deee",
        "captcha_advanced_redirects_trusted_users_deee",
        "block_responses_total_trusted_users_deee",
        "block_responses_marked_total_deee",
        "block_responses_marked_with_captcha_deee",
        "block_responses_marked_with_Lcookie_deee",
        "block_responses_marked_with_trusted_users_deee",
        "block_responses_marked_with_degradation_deee",
        "block_responses_marked_already_banned_deee",
        "block_responses_marked_already_blocked_deee",
        "block_responses_user_marked_total_deee",
        "block_responses_from_yandex_deee",
        "block_responses_from_whitelist_deee",
        "block_responses_user_marked_total_trusted_users_deee",
        "block_responses_user_marked_with_captcha_deee",
        "with_suspiciousness_requests_trusted_users_deee",
        "captcha_redirects_deee",
        "requests_passed_to_service_req_deee",
        "with_degradation_requests_req_deee",
        "with_suspiciousness_requests_req_deee",
        "requests_req_deee",
        "block_responses_total_req_deee",
    ]

    common_signals = [
        "cbb_errors_deee",
        "cbb_queue_size_ahhh",
        "cpu_usage_ahhh",
        "new_ddosers_deee",
        "ddos_num_requests_deee",
        "ddos_num_requests1_deee",
        "ddos_num_requests2_deee",
        "ddos_num_requests3_deee",
        "discovery_updates_deee",
        "discovery_status_ahhh",
        "userbase_hits_deee",
        "userbase_miss_deee",
        "userbase_write_errors_deee",
        "userbase_write_errors_last_deee",
        "userbase_cache_hits_deee",
        "userbase_cache_miss_deee",
        "userbase_count_ahhh",
        "userbase_warning_logs_deee",
        "userbase_error_logs_deee",
        "userbase_cache_size_ahhh",
        "userbase_flush_time_ahhh",
        "userbase_flush_dur_ahhh",
        "userbase_flush_items_ahhh",
        "userbase_flush_dirty_size_ahhh",
        "userbase_sync_errors_deee",
        "userbase_in_use_ahhh",
        "captcha_web_main_redirects_deee",
        "captcha_generation_errors_deee",
        "partners.captcha_redirects_deee",
        "partners.captcha_shows_deee",
        "partners.captcha_image_shows_deee",
        "partners.captcha_correct_inputs_deee",
        "partners.captcha_incorrect_inputs_deee",
        "internal_queue_size_ahhh",
        "read_req_timeouts_deee",
        "page_400_deee",
        "page_404_deee",
        "unknown_service_headers_deee",
        "reply_fails_deee",
        "processor_response_apply_queue.spilled_applies_deee",
        "processing_queue.processing_captcha_inputs.num_tasks_ahhh",
        "processing_queue.processing_captcha_inputs.slow_queue_rps_ahhh",
        "processing_queue.processing_captcha_inputs.slow_processed_deee",
        "processing_queue.processing_captcha_inputs.spilled_reqs_deee",
        "processing_queue.processing_captcha_inputs.slow_sum_time_deee",
        "processing_queue.processing_reqs.num_tasks_ahhh",
        "processing_queue.processing_reqs.slow_queue_rps_ahhh",
        "processing_queue.processing_reqs.slow_processed_deee",
        "processing_queue.processing_reqs.slow_sum_time_deee",
        "processing_queue.processing_reqs.spilled_reqs_deee",
        "processing_queue.hlmq_threads_total_ahhh",
        "processing_queue.hlmq_threads_free_ahhh",
        "processing_queue.active_thread_count_ahhh",
        "processing_queue.hlmq_queue_size_ahhh",
        "processor_response_apply_queue.hlmq_threads_total_ahhh",
        "processor_response_apply_queue.hlmq_threads_free_ahhh",
        "processor_response_apply_queue.active_thread_count_ahhh",
        "processor_response_apply_queue.hlmq_queue_size_ahhh",
        "forward_failures_deee",
        "skip_count_deee",
        "captcha_check_errors_deee",
        "captcha_check_bad_requests_deee",
        "fury_check_errors_deee",
        "fury_preprod_check_errors_deee",
        "stopped_fury_requests_deee",
        "stopped_fury_preprod_requests_deee",
        "http_server.hlmq_threads_total_ahhh",
        "http_server.hlmq_threads_free_ahhh",
        "http_server.active_thread_count_ahhh",
        "http_server.hlmq_queue_size_ahhh",
        "process_server.hlmq_threads_total_ahhh",
        "process_server.hlmq_threads_free_ahhh",
        "process_server.active_thread_count_ahhh",
        "process_server.hlmq_queue_size_ahhh",
        "dtp_got_strong_deee",
        "dtp_killed_threads_deee",
        "dtp_got_ready_deee",
        "dtp_easy_call_deee",
        "dtp_free_threads_deee",
        "dtp_busy_threads_deee",
        "dtp_created_deee",
        "dtp_returned_deee",
        "not_blocked_requests_deee",
        "not_banned_requests_deee",
        "forward_request_1st_try_deee",
        "forward_request_2nd_try_deee",
        "forward_request_3rd_try_deee",
        "forward_request_all_deee",
        "forward_request_exceptions_deee",
        "http_server.active_connections_ahhh",
        "process_server.active_connections_ahhh",
        "captcha_requests_all_deee",
        "captcha_requests_1st_try_deee",
        "captcha_requests_2nd_try_deee",
        "captcha_requests_3rd_try_deee",
        "processings_on_cacher_deee",
        "neh_queue_size_ahhh",
        "captcha_neh_queue_size_ahhh",
        "fury_neh_queue_size_ahhh",
        "cbb_neh_queue_size_ahhh",
        "userbase_is_cleaning_ahhh",
        "userbase_db_get_exceptions_deee",
        "userbase_skipped_reads_deee",
        "invalid_verochka_requests_deee",
        "requests_from_china_not_loginned_deee",
        "redirected_to_login_requests_from_china_deee",
        "unauthorized_request_from_china_deee",
        "is_china_redirect_enabled_ahhh",
        "is_china_unauthorized_enabled_ahhh",
        "bad_requests_count_deee",
        "bad_expects_count_deee",
        "invalid_partner_requests_count_deee",
        "uid_creation_failures_count_deee",
        "other_exceptions_count_deee",
        "valid_autoru_tampers_deee",
        "block_responses_with_unique_Lcookies_ahhh",
        "cacher_exp_formula_id_0.cacher_exp_robots_with_captcha_deee",
        "exp_formula_id_0.exp_robots_with_captcha_deee",
        "processor_daemonlog_records_deee",
        "eventlog_records_deee",
        "eventlog_records_overflow_deee",
        "cacher_daemonlog_records_deee",
        "processor_daemonlog_records_overflow_deee",
        "cacher_daemonlog_records_overflow_deee",
        "cacher_daemonlog_queue_size_ahhh",
        "processor_daemonlog_queue_size_ahhh",
        "eventlog_queue_size_ahhh",
    ]

    antirobot_only_signals = [
        "processings_on_cacher_deee",
        "requests_with_captcha_deee",
        "requests_deee",
        "requests_trusted_users_deee",
        "process_server.hlmq_queue_size_ahhh",
        "ddos_num_requests3_deee",
        "exp_formula_id_0.exp_robots_with_captcha_deee",
        "requests_with_captcha_trusted_users_deee",
        "process_server.active_thread_count_ahhh",
        "requests_req_deee",
        "process_server.hlmq_threads_total_ahhh",
        "process_server.active_thread_count_ahhh",
        "wizard_errors_deee",
        "robots_with_captcha_deee",
        "ddos_num_requests1_deee",
        "whitelist_queries_deee",
        "robots_deee",
        "ddos_num_requests_deee",
        "missed_once_robots_deee",
        "missed_multiple_robots_deee",
        "new_ddosers_deee",
        "cacher_exp_formula_id_0.cacher_exp_robots_with_captcha_deee",
        "process_server.hlmq_threads_free_ahhh",
        "ddos_num_requests2_deee",
        "unique_random_factors_deee",
        "process_server.active_connections_ahhh",
        "marked_by_yql_deee",
        "whitelist_queries_susp_deee",
        "cacher_daemonlog_queue_size_ahhh",
        "processor_daemonlog_queue_size_ahhh",
    ]

    captcha_api_signals = [
        "captcha_key_loads_deee",
        "captcha_validate_valid_secret_requests_deee",
        "captcha_check_api_failed_requests_deee",
        "captcha_iframe_advanced_shows_deee",
        "captcha_validate_success_deee",
        "captcha_iframe_wrong_domain_requests_deee",
        "captcha_validate_invalid_spravka_requests_deee",
        "captcha_validate_failed_deee",
        "captcha_iframe_invalid_sitekey_requests_deee",
        "captcha_validate_unknown_errors_deee",
        "captcha_iframe_allowed_requests_deee",
        "captcha_validate_invalid_secret_requests_deee",
        "captcha_validate_api_failed_requests_deee",
        "captcha_iframe_api_failed_requests_deee",
        "captcha_validate_unauthenticated_requests_deee",
        "captcha_validate_tvm_success_requests_deee",
        "captcha_validate_tvm_unknown_errors_deee",
        "captcha_iframe_checkbox_shows_deee",
        "captcha_validate_requests_deee",
        "captcha_validate_tvm_denied_requests_deee",
        "captcha_check_unauthorized_requests_deee",
        "captcha_check_valid_sitekey_requests_deee",
        "captcha_check_invalid_sitekey_requests_deee",
        "captcha_iframe_suspended_requests_deee",
        "captcha_validate_suspended_requests_deee",
    ]

    exp_bin_signals = [
        "not_banned_requests_cbb_deee",
    ]

    dead_services = [
        "auto",
        "hiliter",
        "kpapi",
        "marketapi_blue",
        "marketred",
        "tech",
        "slovari",
        "rca",
    ]

    id_types = [
        "other",
        "2_spravka",
        "7_lcookie",
        "10_icookie",
    ]

    paths = []

    def append(prefix, signal):
        if is_captcha_api_service and signal in antirobot_only_signals:
            return
        if prefix:
            prefix += ";"
        paths.append(f"{prefix}{signal}")

    for signal in common_signals:
        append("", signal)

    exp_bins = [str(i) for i in range(4)]

    for exp_bin in exp_bins:
        for suffix in exp_bin_signals:
            append(f"exp_bin={exp_bin}", suffix)

    services = []
    for service in service_config:
        service_name = service["service"]
        if is_captcha_api_service and service_name != "other":
            continue

        services.append(service_name)

        for suffix in service_signals:
            append(f"service_type={service_name}", suffix)

        if service_name in dead_services:
            continue

        groups = set(
            group["req_group"]
            for group in service["re_groups"]
        ) | {"generic"}

        for group in groups:
            for suffix in group_signals:
                append(f"service_type={service_name};req_group={group}", suffix)

            for id_type in id_types:
                for suffix in id_group_signals:
                    append(f"service_type={service_name};id_type={id_type};req_group={group}", suffix)

        for exp_bin in exp_bins:
            for suffix in service_exp_bin_signals:
                append(f"service_type={service_name};exp_bin={exp_bin}", suffix)

        for id_type in id_types:
            for suffix in service_id_signals:
                append(f"service_type={service_name};id_type={id_type}", suffix)

            for exp_bin in exp_bins:
                for suffix in service_id_exp_bin_signals:
                    append(f"service_type={service_name};id_type={id_type};exp_bin={exp_bin}", suffix)

    for suffix in id_signals:
        for id_type in id_types:
            append(f"id_type={id_type}", suffix)

    timings_vec = ("0.5ms", "1ms", "5ms", "10ms", "30ms", "50ms", "90ms", "100ms", "10s")
    cacher_timings_vec = ("0.5ms", "1ms", "5ms", "10ms", "30ms", "50ms", "90ms", "100ms", "190ms", "10s")
    large_timings_vec = ("1ms", "5ms", "10ms", "30ms", "50ms", "100ms", "200ms", "300ms", "400ms", "500ms", "700ms", "10s")

    time_signals = [
        f"{prefix}time_{stat_type}{duration}_deee"
        for stat_type in ("", "upper_")
        for duration in cacher_timings_vec
        for prefix in (
            "",
            "read_",
            "wait_",
            "http_server.",
            "http_server.handle_",
            "http_server.read_",
            "http_server.wait_",
            "captcha_",
            "captcha_response_",
            "cacher_request_features_",
            "cacher_formula_calc_",
        )
    ] + [
        f"{prefix}time_{stat_type}{duration}_deee"
        for stat_type in ("", "upper_")
        for duration in timings_vec
        for prefix in (
            "process_server.",
            "process_server.handle_",
            "process_server.read_",
            "process_server.wait_",
            "processing_",
            "forward_request_single_",
            "forward_request_with_retries_",
            "wizard_calc_",
            "factors_calc_",
            "factors_processing_",
            "formula_calc_",
            "exp_formulas_calc_",
            "cacher_exp_formulas_calc_",
        )
    ] + [
        f"service_type={service};{prefix}time_{stat_type}{duration}_deee"
        for stat_type in ("", "upper_")
        for duration in cacher_timings_vec
        for service in services
        for prefix in (
            "handle_",
        )
    ] + [
        f"{prefix}time_{stat_type}{duration}_deee"
        for stat_type in ("", "upper_")
        for duration in large_timings_vec
        for prefix in (
            "api_captcha_check_response_",
            "fury_check_response_",
            "fury_preprod_check_response_",
        )
    ]

    if is_captcha_api_service:
        time_signals = [
            signal
            for signal in time_signals
            if not re.search("^(process_server|wizard|factors|exp_formulas|cacher_exp|formula_calc)", signal)
        ]
        paths += captcha_api_signals

    paths += time_signals

    return paths
