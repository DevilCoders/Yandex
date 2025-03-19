#!/usr/bin/env python2
# -*- coding: utf-8 -*-
import datetime
date_start = str(datetime.date.today()-datetime.timedelta(days = 1))
date_end = str(datetime.date.today())
queries = {
    'visits_sources': '''
    SELECT
        CounterID as counter_id,
        UserID as user_id,
        VisitID as visit_id,
        VisitVersion as visit_version,
        StartTime as  start_time,
        toDateTime((toInt64(UTCStartTime) - 10800)) as event_time,
        Referer as referer,
        multiIf(
            UTMSource IS NOT NULL AND UTMSource != '', UTMSource,
            TraficSourceID == 3, 'perfomance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            'unknown'
        ) as utm_source,
        multiIf(
            UTMMedium IS NOT NULL AND UTMMedium != '', UTMMedium,
            TraficSourceID == 3, 'perfomance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            'unknown'
        ) as utm_medium,
        multiIf(
            UTMCampaign IS NOT NULL AND UTMCampaign != '', UTMCampaign,
            TraficSourceID == 3, 'perfomance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            'unknown'
        ) as utm_campaign,
        UTMTerm as utm_term,
        UTMContent as utm_content,
        SearchPhrase as search_phrase,
        multiIf(
            UTMSource IN ('YD_Search_Rus', 'a_share', 'amp_eskimobi'), 'Unknown',
            UTMSource IN ('emailing', 'emailings'), 'Emailing',
            UTMSource IN ('koldunzhik', 'koldunzhik2', 'objectanswer'), 'Yandex Portal',
            UTMSource IN ('ok', 'tg', 'tw', 'vk', 'facebook', 'fb'), 'Social',
            UTMSource IN ('yadirect'), 'Perfomance',
            UTMSource IN ('google'), 'Perfomance',
            UTMSource IN ('GA_Display_Rus'), 'Perfomance',
            UTMSource IN ('YD_Display_Rus'), 'Perfomance',
            UTMSource =='youtube' and UTMMedium == 'paidvideo', 'Perfomance',
            UTMSource =='ymainteaser',  'Yandex Portal',
            UTMSource =='telegram',  'Perfomance',
            UTMSource IN ('habr', 'habrahabr', 'radiot', 'referral', 'sender', 'tech.yandex.ru', 'techblog', 'techyandexru', 'dialogsblog','youtube', 'ywmmenu', 'yxnews', 'zen', 'wmblog', 'yandex'), 'Referrals',
            TraficSourceID == 3, 'Perfomance',
            TraficSourceID == 5, 'Unknown',
            TraficSourceID == 2, 'Organic Search',
            TraficSourceID == 8, 'Social',
            TraficSourceID == 1, 'Referrals',
            TraficSourceID == 0, 'Direct',
            TraficSourceID == -1, 'AutoReferrals',
            TraficSourceID == 4, 'Direct',
            'Other'
        ) as channel,
        multiIf(
            UTMSource IN ('YD_Search_Rus', 'a_share', 'amp_eskimobi'), 'Не определен',
            UTMSource IN ('emailing', 'emailings'), 'Переходы из почтовых рассылок',
            UTMSource IN ('koldunzhik', 'koldunzhik2', 'objectanswer'), 'Поиск(метки)',
            UTMSource IN ('ok', 'tg', 'tw', 'vk', 'facebook', 'fb'), 'Переходы из социальных сетей',
            UTMSource IN ('yadirect'), 'Переходы по рекламе (Директ)',
            UTMSource IN ('google'), 'Переходы по рекламе (AdWords)',
            UTMSource IN ('GA_Display_Rus'), 'Переходы по рекламе (GDN)',
            UTMSource IN ('YD_Display_Rus'), 'Переходы по рекламе (РСЯ)',
            UTMSource =='youtube' and UTMMedium == 'paidvideo', 'Переходы по рекламе (Youtube)',
            UTMSource =='ymainteaser',  'Переходы по рекламе (Морда)',
            UTMSource =='telegram',  'Переходы по рекламе (Телеграмм)',
            UTMSource IN ('habr', 'habrahabr', 'radiot', 'referral', 'sender', 'tech.yandex.ru', 'techblog', 'techyandexru', 'dialogsblog','youtube', 'ywmmenu', 'yxnews', 'zen', 'wmblog', 'yandex'), 'Переходы по ссылкам на сайтах',
            TraficSourceID == 3, 'Переходы по рекламе (другое)',
            TraficSourceID == 5, 'Не определен',
            TraficSourceID == 2, 'Поиск',
            TraficSourceID == 8, 'Переходы из социальных сетей (другое)',
            TraficSourceID == 1, 'Переходы по ссылкам на сайтах (другое)',
            TraficSourceID == 0, 'Прямые заходы',
            TraficSourceID == -1, 'Внутренние переходы',
            TraficSourceID == 4, 'Переходы с сохранённых страниц',
            'Другое'
        ) as channel_detailed,
        AdBlock as ad_block,
        ResolutionDepth as resolution_depth,
        IsRobot as is_robot,
        StartURL as start_url
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '2018-11-01'
        AND NOT match(Referer, '(yandex-team|e.biz.yandexcloud.net)')
        AND Sign == 1
    FORMAT TabSeparatedWithNames
    ''',

    'visits_region_visit_options': '''
    SELECT
        CounterID as counter_id,
        VisitID as visit_id,
        VisitVersion as visit_version,
        TotalVisits as total_visits,
        Duration as duration,
        PageViews as page_views,
        Hits as hits,
        IsBounce as is_bounce,
        regionToName(regionToCountry(RegionID)) as country,
        regionToName(regionToArea(RegionID)) as area,
        regionToName(regionToCity(RegionID)) as city,
        ClientIP as client_ip,
        RemoteIP as remote_ip,
        ResolutionWidth as resolution_width,
        ResolutionHeight as resolution_height,
        WindowClientWidth as window_client_width,
        WindowClientHeight as window_client_height,
        dictGetString('OS', 'value', toUInt64(OS)) AS os
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '2018-11-01'
        AND NOT match(Referer, '(yandex-team|e.biz.yandexcloud.net)')
        AND Sign == 1
    FORMAT TabSeparatedWithNames
    ''',

    'visits_tech_info': '''
    SELECT
        CounterID as counter_id,
        VisitID as visit_id,
        VisitVersion as visit_version,
        multiIf(
            IsTablet == 1, 'tablet',
            IsTV == 1, 'tv',
            IsMobile == 1, 'mobile',
            'desktop'
        ) as device_type,
        MobilePhoneVendor as mobile_phone_vendor,
        DeviceModel as device_model,
        multiIf(
            Age == 0, 'undefined',
            Age == 17, '0-17',
            Age == 18, '18-24',
            Age == 25, '24-34',
            Age == 35, '35-44',
            Age == 45, '45-54',
            Age == 55, '55+',
            'undefined'
        ) as age,
        multiIf(
            Sex == 0, 'undefined',
            Sex == 1, 'male',
            Sex == 2, 'female',
            'undefined'
        ) as sex,
        Income as income,
        Interests as interests,
        GeneralInterests as general_interests,
        FirstVisit as first_visit_dt,
        'visit' as event
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '2018-11-01'
        AND NOT match(Referer, '(yandex-team|e.biz.yandexcloud.net)')
        AND Sign == 1
    FORMAT TabSeparatedWithNames
    ''',

    'visits_yandexuid_puid_dict': '''
    SELECT
        v.counter_id,
        v.user_id,
        v.visit_id,
       h.puid as puid
    FROM(
        SELECT
            CounterID as counter_id,
            UserID as user_id,
            VisitID as visit_id,
            watch_id
        FROM
            visits_all
        ARRAY JOIN WatchIDs AS watch_id
        WHERE
            counter_id IN (50027884, 48570998)
            AND StartDate >= '2018-11-01'
        ) as v
    ALL INNER JOIN (
        SELECT
            CounterID as counter_id,
            UserID as user_id,
            WatchID as watch_id,
            PassportUserID as puid
        FROM
            hits_all
        WHERE
            counter_id IN (50027884, 48570998)
            AND EventDate >= '2018-11-01'
            AND puid > 0
        GROUP BY
            counter_id,
            user_id,
            watch_id,
            puid
    ) AS h
    ON v.counter_id == h.counter_id AND v.user_id == h.user_id AND h.watch_id == v.watch_id
    GROUP BY
        counter_id,
        user_id,
        visit_id,
        puid
    FORMAT TabSeparatedWithNames
    ''',
}

queries_append_mode = {
    'visits_sources': '''
    SELECT
        CounterID as counter_id,
        UserID as user_id,
        VisitID as visit_id,
        VisitVersion as visit_version,
        StartTime as  start_time,
        toDateTime((toInt64(UTCStartTime) - 10800)) as event_time,
        Referer as referer,
        multiIf(
            UTMSource IS NOT NULL AND UTMSource != '', UTMSource,
            TraficSourceID == 3, 'perfomance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            'unknown'
        ) as utm_source,
        multiIf(
            UTMMedium IS NOT NULL AND UTMMedium != '', UTMMedium,
            TraficSourceID == 3, 'perfomance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            'unknown'
        ) as utm_medium,
        multiIf(
            UTMCampaign IS NOT NULL AND UTMCampaign != '', UTMCampaign,
            TraficSourceID == 3, 'perfomance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            'unknown'
        ) as utm_campaign,
        UTMTerm as utm_term,
        UTMContent as utm_content,
        SearchPhrase as search_phrase,
        multiIf(
            UTMSource IN ('YD_Search_Rus', 'a_share', 'amp_eskimobi'), 'Unknown',
            UTMSource IN ('emailing', 'emailings'), 'Emailing',
            UTMSource IN ('koldunzhik', 'koldunzhik2', 'objectanswer'), 'Yandex Portal',
            UTMSource IN ('ok', 'tg', 'tw', 'vk', 'facebook', 'fb'), 'Social',
            UTMSource IN ('yadirect'), 'Perfomance',
            UTMSource IN ('google'), 'Perfomance',
            UTMSource IN ('GA_Display_Rus'), 'Perfomance',
            UTMSource IN ('YD_Display_Rus'), 'Perfomance',
            UTMSource =='youtube' and UTMMedium == 'paidvideo', 'Perfomance',
            UTMSource =='ymainteaser',  'Yandex Portal',
            UTMSource =='telegram',  'Perfomance',
            UTMSource IN ('habr', 'habrahabr', 'radiot', 'referral', 'sender', 'tech.yandex.ru', 'techblog', 'techyandexru', 'dialogsblog','youtube', 'ywmmenu', 'yxnews', 'zen', 'wmblog', 'yandex'), 'Referrals',
            TraficSourceID == 3, 'Perfomance',
            TraficSourceID == 5, 'Unknown',
            TraficSourceID == 2, 'Organic Search',
            TraficSourceID == 8, 'Social',
            TraficSourceID == 1, 'Referrals',
            TraficSourceID == 0, 'Direct',
            TraficSourceID == -1, 'AutoReferrals',
            TraficSourceID == 4, 'Direct',
            'Other'
        ) as channel,
        multiIf(
            UTMSource IN ('YD_Search_Rus', 'a_share', 'amp_eskimobi'), 'Не определен',
            UTMSource IN ('emailing', 'emailings'), 'Переходы из почтовых рассылок',
            UTMSource IN ('koldunzhik', 'koldunzhik2', 'objectanswer'), 'Поиск(метки)',
            UTMSource IN ('ok', 'tg', 'tw', 'vk', 'facebook', 'fb'), 'Переходы из социальных сетей',
            UTMSource IN ('yadirect'), 'Переходы по рекламе (Директ)',
            UTMSource IN ('google'), 'Переходы по рекламе (AdWords)',
            UTMSource IN ('GA_Display_Rus'), 'Переходы по рекламе (GDN)',
            UTMSource IN ('YD_Display_Rus'), 'Переходы по рекламе (РСЯ)',
            UTMSource =='youtube' and UTMMedium == 'paidvideo', 'Переходы по рекламе (Youtube)',
            UTMSource =='ymainteaser',  'Переходы по рекламе (Морда)',
            UTMSource =='telegram',  'Переходы по рекламе (Телеграмм)',
            UTMSource IN ('habr', 'habrahabr', 'radiot', 'referral', 'sender', 'tech.yandex.ru', 'techblog', 'techyandexru', 'dialogsblog','youtube', 'ywmmenu', 'yxnews', 'zen', 'wmblog', 'yandex'), 'Переходы по ссылкам на сайтах',
            TraficSourceID == 3, 'Переходы по рекламе (другое)',
            TraficSourceID == 5, 'Не определен',
            TraficSourceID == 2, 'Поиск',
            TraficSourceID == 8, 'Переходы из социальных сетей (другое)',
            TraficSourceID == 1, 'Переходы по ссылкам на сайтах (другое)',
            TraficSourceID == 0, 'Прямые заходы',
            TraficSourceID == -1, 'Внутренние переходы',
            TraficSourceID == 4, 'Переходы с сохранённых страниц',
            'Другое'
        ) as channel_detailed,
        AdBlock as ad_block,
        ResolutionDepth as resolution_depth,
        IsRobot as is_robot,
        StartURL as start_url
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '{0}'
        AND StartDate < '{1}'
        AND NOT match(Referer, '(yandex-team|e.biz.yandexcloud.net)')
        AND Sign == 1
    FORMAT TabSeparatedWithNames
    '''.format(date_start, date_end),

    'visits_region_visit_options': '''
    SELECT
        CounterID as counter_id,
        VisitID as visit_id,
        VisitVersion as visit_version,
        TotalVisits as total_visits,
        Duration as duration,
        PageViews as page_views,
        Hits as hits,
        IsBounce as is_bounce,
        regionToName(regionToCountry(RegionID)) as country,
        regionToName(regionToArea(RegionID)) as area,
        regionToName(regionToCity(RegionID)) as city,
        ClientIP as client_ip,
        RemoteIP as remote_ip,
        ResolutionWidth as resolution_width,
        ResolutionHeight as resolution_height,
        WindowClientWidth as window_client_width,
        WindowClientHeight as window_client_height,
        dictGetString('OS', 'value', toUInt64(OS)) AS os
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '{0}'
        AND StartDate < '{1}'
        AND NOT match(Referer, '(yandex-team|e.biz.yandexcloud.net)')
        AND Sign == 1
    FORMAT TabSeparatedWithNames
    '''.format(date_start, date_end),

    'visits_tech_info': '''
    SELECT
        CounterID as counter_id,
        VisitID as visit_id,
        VisitVersion as visit_version,
        multiIf(
            IsTablet == 1, 'tablet',
            IsTV == 1, 'tv',
            IsMobile == 1, 'mobile',
            'desktop'
        ) as device_type,
        MobilePhoneVendor as mobile_phone_vendor,
        DeviceModel as device_model,
        multiIf(
            Age == 0, 'undefined',
            Age == 17, '0-17',
            Age == 18, '18-24',
            Age == 25, '24-34',
            Age == 35, '35-44',
            Age == 45, '45-54',
            Age == 55, '55+',
            'undefined'
        ) as age,
        multiIf(
            Sex == 0, 'undefined',
            Sex == 1, 'male',
            Sex == 2, 'female',
            'undefined'
        ) as sex,
        Income as income,
        Interests as interests,
        GeneralInterests as general_interests,
        FirstVisit as first_visit_dt,
        'visit' as event
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '{0}'
        AND StartDate < '{1}'
        AND NOT match(Referer, '(yandex-team|e.biz.yandexcloud.net)')
        AND Sign == 1
    FORMAT TabSeparatedWithNames
    '''.format(date_start, date_end),

    'visits_yandexuid_puid_dict': '''
    SELECT
        v.counter_id,
        v.user_id,
        v.visit_id,
       h.puid as puid
    FROM(
        SELECT
            CounterID as counter_id,
            UserID as user_id,
            VisitID as visit_id,
            watch_id
        FROM
            visits_all
        ARRAY JOIN WatchIDs AS watch_id
        WHERE
            counter_id IN (50027884, 48570998)
            AND StartDate >= '2018-09-01'
        ) as v
    ALL INNER JOIN (
        SELECT
            CounterID as counter_id,
            UserID as user_id,
            WatchID as watch_id,
            PassportUserID as puid
        FROM
            hits_all
        WHERE
            counter_id IN (50027884, 48570998)
            AND EventDate >= '2018-09-01'
            AND puid > 0
        GROUP BY
            counter_id,
            user_id,
            watch_id,
            puid
    ) AS h
    ON v.counter_id == h.counter_id AND v.user_id == h.user_id AND h.watch_id == v.watch_id
    GROUP BY
        counter_id,
        user_id,
        visit_id,
        puid
    FORMAT TabSeparatedWithNames
    '''.format(date_start, date_end),
}
