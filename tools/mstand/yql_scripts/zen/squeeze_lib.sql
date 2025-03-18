$parse_proto = YQL::Udf(AsAtom("Protobuf.TryParse"), YQL::Void(), YQL::Void(),
    AsAtom(@@{"name":"ItemSecData","lists":{"optional":false},"meta":"H4sIAAAAAAAAAFWNOw7CMBBElYTwWSjAonBBEajCMfgVKaIUwAFMvASL2I7sBSm34wgciUBHOU9vZuAdwDgj1Ecs94LE6vWf2QxGpDR6ErrhQRKmEWMAjbPakrLG8zCJ0pgtYF6L1j5oJ4xUUhD6U9sg73WNmK1h6bC0WqOR6P4Vv1WUC3/n/d9493etVXWjTBZ8kAQdmsOEhKuQDk80VPBhR2PGYXpRcuNzVTp7Nop8wUdf/wM3STSY0wAAAA=="}@@));

$get_rct_id = ($secdata) -> {
    $rct_id = $parse_proto(String::Base64Decode($secdata)).layoutCandidatesType;
    return $rct_id;
};

define subquery $bg_sessions($dt, $cluster) as
    $path = '//home/recommender/zen/sessions_aggregates/background/' || cast($dt as string);
    select strongestId, experiments, userTemperature from yt:$cluster.$path;
end define;

define subquery $log($dt, $cluster) as
    $path = '//logs/zen-events-checked-log/1d/' || cast($dt as string);
    select a.*, cast($dt as date) as dt from yt:$cluster.$path with columns Struct<_rest:Yson> as a;
end define;

define subquery $user_testids($dt, $testids, $cluster) as
    select strongest_id, aggregate_list(distinct testid) as testids
    from $bg_sessions($dt, $cluster)
    flatten list by (Yson::ConvertToStringList(experiments) as testid)
    where ListHas($testids, testid)
    group by strongestId as strongest_id;
end define;

define subquery $decode_testids($testids, $cluster) as
    select id, if(ListHas($testids, id), cast(id as string), experimentName) as experimentName
    from yt:$cluster.`//home/recommender/zen/export/experiment2id`
    where ListHas($testids, experimentName) or ListHas($testids, id);
end define;

$parse_experiments = ($experiments) -> (
    ListMap(String::SplitToList($experiments, ','), ($x) -> (if(String::StartsWith($x, 'ab:'), 'rec:' || $x, $x)))
);

$filter_experiments = ($experiments, $testids) -> (
    ListFilter($experiments, ($x) -> (ListHas($testids, $x)))
);

$decode_group_ids = ($group_ids, $match_testids) -> (
    ListNotNull(ListMap($group_ids, ($x) -> ($match_testids[$x])))
);

$convert_group_ids = ($group_ids) -> (
    ListMap(Yson::ConvertToList($group_ids), ($x)->(Yson::ConvertToInt64($x, Yson::Options(false as Strict))))
);

$process_exps = ($experiments, $group_ids, $match_testids) -> (
    ListUniq(
        ListExtend(
            $filter_experiments(
                $parse_experiments($experiments),
                DictPayloads($match_testids)
            ),
            $decode_group_ids($group_ids, $match_testids)
        )
    )
);

define subquery $squeeze($dt, $cluster, $testids) as
    $session_length = 15 * 60;

    $white_list = [
        'action',
        'ad_feed:click',
        'ad_feed:show',
        'autopause',
        'autoplay',
        'call2action:click',
        'call2action:show',
        'click',
        'comments:counter_click',
        'complain:click',
        'complain:show',
        'content_item:click',
        'content_item:less',
        'content_item:show',
        'end',
        'favourite_button:show',
        'feedback:block',
        'feedback:favourite',
        'feedback:less',
        'feedback:more',
        'interview:show',
        'interview:submit',
        'pause',
        'play',
        'replay',
        'rtb_inserts:click',
        'rtb_inserts:show',
        'share',
        'show',
        'similar:click',
        'sound_off',
        'sound_on',
        'source:click',
        'source:share',
        'source:show',
        'swipe',
        'view',
    ];

    $heartbeat_events = [
        'deepwatch',
        'heartbeat',
    ];

    $events = [
        'ad_similar:click',
        'ad_similar:show',
        'auth:click',
        'auth:login',
        'auth:show',
        'autoplay_off',
        'autoplay_on',
        'comments:author_select:close',
        'comments:author_select:select',
        'comments:author_select:show',
        'comments:blocked',
        'comments:cancel_delete',
        'comments:cancel_dislike',
        'comments:cancel_like',
        'comments:captcha:fail',
        'comments:captcha:ok',
        'comments:change_sorting',
        'comments:complain',
        'comments:copy_link',
        'comments:counter_show',
        'comments:create',
        'comments:delete',
        'comments:dislike',
        'comments:edit',
        'comments:feed_show',
        'comments:image:upload',
        'comments:interface_show',
        'comments:like',
        'comments:likers:list',
        'comments:menu_tip:close',
        'comments:menu_tip:show',
        'comments:next',
        'comments:notification_open',
        'comments:prev',
        'comments:show',
        'custom_user_feedback:cancel_click',
        'custom_user_feedback:click',
        'custom_user_feedback:show',
        'eula:click',
        'eula:show',
        'external_click',
        'feedback:cancel_block',
        'feedback:cancel_favourite',
        'feedback:cancel_less',
        'feedback:cancel_more',
        'forgot_user',
        'iceboarding:cancel_select',
        'iceboarding:click',
        'iceboarding:close',
        'iceboarding:select',
        'iceboarding:show',
        'menu:block',
        'menu:cancel_block',
        'menu:cancel_favourite',
        'menu:favourite',
        'navigate',
        'notification:click',
        'notification:more',
        'notification:settings',
        'onboarding:like',
        'onboarding:show',
        'partner:click',
        'profile:set_country',
        'similar:show',
        'social:about:update',
        'social:activity',
        'social:document_likers:list',
        'social:onboarding:complete',
        'social:onboarding:shown',
        'social:privacy:update',
        'social:profile:open',
        'swipe_to_site',
        'tip:click',
        'tip:show',
        'video:show',
    ];

    $preview_events = [
        'preview:show',
        'preview:click',
        'short',
        'pull',
    ];

    $ad_events = [
        'ad_feed:click',
        'ad_feed:show',
        'rtb_inserts:click',
        'rtb_inserts:show',
    ];

    $moscow_date = ($eventtime) -> (CAST(CAST($eventtime + 3600*3 AS DateTime) AS Date));

    $check_date = ($ts, $file_date) -> (if($moscow_date($ts) >= $file_date - Interval('P2D') and $moscow_date($ts) <= $file_date, $ts, null));

    $choose_ts = ($client_ts_norm, $client_ts, $ts, $file_date) ->
        ($check_date($client_ts_norm / 1000, $file_date) ??
        $check_date($client_ts / 1000, $file_date) ??
        $check_date($ts / 1000, $file_date));

    $generate_session = ($strongest_id, $ts) -> ($strongest_id || '/' || cast($ts as String));

    $grid_width = ($grid_type) -> (
        {
            null: 1,
            'three_column': 3,
            'one_column': 1,
            'two_column': 2,
            'three_column_infinity': 3,
            'two_column_small_cards': 2,
            'three_column_small_cards': 3
        }[$grid_type]
    );

    $card_width = ($pos, $grid_type) -> (
        case
            when $pos is null or $grid_type is null then null
            when ListHas(['one_column', 'two_column_small_cards', 'three_column_small_cards'], $grid_type) then 1.
            when $grid_type = 'two_column' then if($pos % 4 = 2 or $pos % 4 = 3, 2., 1.)
            when ListHas(['three_column', 'three_column_infinity'], $grid_type) then if($pos % 7 = 3 or $pos % 7 = 6, 2., 1.)
            else null
        end
    );

    $match_testids = select ToDict(agg_list((id, experimentName))) from $decode_testids($testids, $cluster);

    $src = select
        a.strongest_id as strongest_id,
        $choose_ts(client_ts_norm, client_ts, ts, dt) as ts,
        testids,
        device_id,
        event,
        integration,
        Yson::LookupBool(data, 'is_short', Yson::Options(false as Strict)) as is_short,
        Yson::LookupString(data, 'service_action', Yson::Options(false as Strict)) as service_action,
        Yson::LookupBool(data, 'is_complete_read', Yson::Options(false as Strict)) as is_complete_read,
        case
            when item_type = 'interview' then origin_id
            else item_id
        end as item_id,
        item_id as real_item_id,
        case
            when item_type = 'interview' then origin_type
            when ListHas($ad_events, event) then 'ad'
        else item_type end as item_type,
        item_type as real_item_type,
        parent_id,
        parent_type,
        page_type,
        partner,
        place,
        case
            when event = 'view' and page_type is null then null
            when item_type = 'interview' and origin_id is null then 9999ul
            else pos end
        as pos,
        product,
        $choose_ts(client_ts_norm, client_ts, ts, dt) as real_timestamp,
        rid as real_rid,
        Yson::ConvertToInt64List(_rest['answer_ids'], Yson::Options(false as Strict, true as AutoConvert)) as answer_ids,
        Yson::ConvertToString(_rest['forced_rid'], Yson::Options(false as Strict)) ?? rid as rid,
        card_type as real_card_type,
        source_id,
        source_type,
        uid,
        String::ReplaceAll(url, 'http://example.com', 'http://zen.yandex.ru') as url,
        yandexuid,
        card_type,
        origin_id,
        origin_type,
        grid_type,
        placeholder_type,
        case
            when event = 'deepwatch' then cast(time as integer) ?? 4
            when event = 'heartbeat' then cast(video_position_sec as integer) ?? 0
            else null
        end as raw_dwell_time,
        item_height,
        $get_rct_id(rec_secdata) as rct_id,
        flight_id,
        $process_exps(experiments, $convert_group_ids(group_ids), $match_testids) as applied_testids,
    from $log($dt, $cluster) as a
    join $user_testids($dt, $testids, $cluster) as b using (strongest_id)
    where (ListHas($white_list, event) or ListHas($events, event) or ListHas($preview_events, event) or ListHas($heartbeat_events, event))
    and a.strongest_id is not null and ListLength(Yson::ConvertToList(rules)) = 0
    and $choose_ts(client_ts_norm, client_ts, ts, dt) is not null
    and not (product = 'zen_lib' and partner = 'desktop_morda' and integration = 'morda_zen_lib' and event = 'preview:show');

    $heartbeats = select
        'deepwatch' as event,
        a.* without event
    from (select min_by(TableRow(), ts), max(raw_dwell_time) as dwell_time from $src
    where ListHas($heartbeat_events, event) and item_id is not null
    group by strongest_id, rid, item_type, item_id, page_type ?? '' = 'video_page') as a flatten columns;

    $all = select * from $src as a where not ListHas($events, event) and not ListHas($heartbeat_events, event)
    union all
    select * from $heartbeats;

    $data_for_sessions = select strongest_id, product, ts, event, pos, ListHas($preview_events, event) as weak
    from $src;

    -- состояние: время последнего события, время последнего сильного события
    -- если видим сильное событие, отстоящее от прошлого сильного (или null) на session_length, начинаем сильную сессию, время начала пишем ts-session_length
    -- если видим слабое событие, отстоящее от прошлого слабого на session_length, начинаем слабую сессию (возможно, она будет задним числом перекрыта сильной)
    -- если видим слабое событие, отстоящее от прошлого сильного (не null) на session_length, тоже начинаем слабую сессию и очищаем время последнего сильного события
    -- потом отсортируем по времени и подольем в события
    -- если видим слабую сессию, перед которой была сильная за <=session_length, то не учитываем
    $sessions = select strongest_id, min(weak) as weak, if(min(weak), SessionStart(), SessionStart() - $session_length) as ts,
    $generate_session(strongest_id, SessionStart()) as session_id
    from $data_for_sessions
    group by strongest_id, SessionWindow(ts,
    ($row)->(($row.ts, if(not $row.weak, $row.ts))),
    ($row, $state)->((Unwrap(not $row.weak and ($state.1 is null or $row.ts - $state.1 > $session_length) or
                            $row.weak and $row.ts - $state.0 > $session_length or
                            $row.weak and $state.1 is not null and $row.ts - $state.1 > $session_length),
                ($row.ts, if($row.weak, if($state.1 is not null and $row.ts - $state.1 > $session_length, null, $state.1), $row.ts)))),
    ($_, $state)->($state.0));

    $with_sessions = select * from (
    select if(last_value(if(not weak, ts)) ignore nulls over w + $session_length >= last_value(if(session_id is not null, ts)) ignore nulls over w,
    last_value(if(not weak, session_id)) ignore nulls over w, last_value(session_id) ignore nulls over w) as session_id,
    a.* without session_id
    from (
    select *
    from $all
    union all
    select * from $sessions) as a
    window w as (partition by strongest_id order by ts))
    where weak is null;

    $rids = select strongest_id, rid, row_number() over w - 1 as rid_rank from (
    select strongest_id, rid, min(ts) as ts
    from $src
    group by strongest_id, rid)
    window w as (partition by strongest_id order by ts);

    $gif = select id, durationSeconds, hasSound from yt:$cluster.`//home/zen/dyntables/content/daos/gif`;

    $src2 =
    select a.*, g.durationSeconds as video_duration_seconds, g.hasSound as video_has_sound
    from (select * from $with_sessions where item_type = 'gif') as a left join any $gif as g on a.item_id = g.id
    union all
    select * from $with_sessions where item_type ?? '' != 'gif';

    $interviews_info = select id as interview_id, Yson::LookupInt64(info, 'id', Yson::Options(false as Strict)) as answer_id,
    Yson::LookupInt64(info, 'positive_ratio', Yson::Options(false as Strict)) as positive_ratio
    from yt:$cluster.`//home/zen/dyntables/interviews_info`
    flatten list by (Yson::LookupList(single_choice_screen_info ?? gradation_choice_image_screen_info, 'answers', Yson::Options(false as Strict)) as info)
    where Yson::LookupInt64(info, 'positive_ratio', Yson::Options(false as Strict)) is not null;

    $with_sessions3 = select
    Yson::ConvertToString(userTemperature, Yson::Options(false as Strict)) as user_age,
    if(a.event = 'interview:submit' and ListLength(a.answer_ids) = 1, b.positive_ratio) as positive_ratio,
    r.rid_rank * 10000 + a.pos as full_pos,
    a.*
    from $src2 as a
    left join any $rids as r on a.strongest_id = r.strongest_id and a.rid = r.rid
    left join any $interviews_info as b on a.item_id = b.interview_id and a.answer_ids[0] = b.answer_id
    left join any $bg_sessions($dt, $cluster) as bs on a.strongest_id = bs.strongestId;

    select
        strongest_id as yuid,
        ts,
        ts as action_index,
        null as bucket,
        testids,
        applied_testids,
        'zen' as servicetype,
        device_id,
        dwell_time,
        case
            when ListHas(['short', 'preview:show', 'ad_feed:show', 'rtb_inserts:show'], event) then 'show'
            when ListHas(['pull', 'preview:click', 'ad_feed:click', 'rtb_inserts:click'], event) then 'click'
            else event
        end as event,
        event as real_event,
        integration,
        is_short,
        item_id,
        real_item_id,
        item_type,
        real_item_type,
        parent_id,
        parent_type,
        page_type,
        partner,
        place,
        product,
        full_pos as pos,
        pos as real_pos,
        ts as real_timestamp,
        rid,
        real_rid,
        card_type as real_card_type,
        session_id,
        source_id,
        uid,
        url,
        user_age,
        yandexuid,
        if(ListHas($preview_events, event), 'preview', 'regular') as card_type,
        item_height,
        $grid_width(grid_type) as grid,
        grid_type,
        $card_width(pos, grid_type) as card_width,
        if(item_type = 'interview', item_id) as interview_id,
        answer_ids,
        positive_ratio,
        is_complete_read ?? false as is_complete_read,
        String::SplitToList(service_action, ':')[0] as ecom_link_type,
        substring(String::SplitToList(service_action, ':')[1], null, 24) as ecom_link_id,
        video_duration_seconds,
        video_has_sound,
        rct_id,
        flight_id,
    from $with_sessions3 as a
    order by yuid, ts, action_index;
end define;

export $squeeze;
