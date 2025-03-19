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
            TraficSourceID == 3, 'performance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            TraficSourceID == 9, 'recommender systems',
            TraficSourceID == 10, 'messengers',
            'unknown'
        ) as utm_source,
        multiIf(
            UTMMedium IS NOT NULL AND UTMMedium != '', UTMMedium,
            TraficSourceID == 3, 'performance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            TraficSourceID == 9, 'recommender systems',
            TraficSourceID == 10, 'messengers',
            'unknown'
        ) as utm_medium,
        multiIf(
            UTMCampaign IS NOT NULL AND UTMCampaign != '', UTMCampaign,
            TraficSourceID == 3, 'performance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            TraficSourceID == 9, 'recommender systems',
            TraficSourceID == 10, 'messengers',
            'unknown'
        ) as utm_campaign,
        UTMTerm as utm_term,
        UTMContent as utm_content,
        SearchPhrase as search_phrase,
        multiIf(
            UTMSource IN ('amp_eskimobi', 'exs', 'share2'), 'Unknown',
            UTMSource = 'koldunshik' AND UTMMedium = 'cpc', 'Yandex Portal',
            UTMMedium IN ('cpc', 'paidpost', 'cpm'), 'Performance',
            UTMSource IN ('facebook','fb','tw','vk', 'ok', 'twitter.com', 'facebook.com', 'vk.com'), 'Social (organic)',
            UTMSource IN ('t.me/ppc_analytics','telegram','telergam','tg', 'telegram.me', 'telegram.me'), 'Messengers',
            UTMSource = 'telegram' AND UTMMedium = 'yacloudnews', 'Telegram (Yandex.Cloud)',
            UTMSource IN ('bno','koldunzhik','objectanswer','teasermain','yandex','yaserp','ymainteaser'), 'Yandex Portal',
            UTMSource IN ('dialogsblog','habr','habr','habr','habrahabr','referrals','Searchengines.ru','tech.yandex.ru','techyandexru','techyandexru','techyandexru','vc','wmblog','yametrika','yametrika','ywmmenu', 'tproger_ru', 'zen', 'techblog', 'ywebmasterblog', 'sender', 'r-point', 'retail_conf'), 'Referrals',
            UTMSource IN ('emailing', 'emailings', 'emailing^'), 'Emailing',
            UTMSource IN ('console', 'banner-console', 'scale2019', 'console-en', 'cloud.yandex', 'banner-site', 'blog-site-link', 'fb_from_scale2019', 'tg_from_scale2019', 'vk_from_scale2019', 'tw_from_scale2019'), 'YCloud Site',
            UTMMedium != '' OR UTMSource != '', 'Performance',
            TraficSourceID == 3, 'Performance',
            TraficSourceID == 5, 'Unknown',
            TraficSourceID == 2, 'Organic Search',
            TraficSourceID == 8, 'Social (organic)',
            TraficSourceID == 1, 'Referrals',
            TraficSourceID == 0, 'Direct',
            TraficSourceID == -1, 'AutoReferrals',
            TraficSourceID == 4, 'Direct',
            TraficSourceID == 9, 'Recommender Systems',
            TraficSourceID == 10, 'Messengers',
            'Other'
        ) as channel,
        multiIf(
            UTMSource IN ('amp_eskimobi', 'exs', 'share2'), 'Unknown',
            UTMSource = 'koldunshik' AND UTMMedium = 'cpc', 'Yandex Portal',
            UTMMedium IN ('cpc', 'paidpost', 'cpm'), 'Performance',
            UTMSource IN ('facebook','fb','tw','vk', 'ok', 'twitter.com', 'facebook.com', 'vk.com'), 'Social (organic)',
            UTMSource IN ('t.me/ppc_analytics','telegram','telergam','tg', 'telegram.me', 'telegram.me'), 'Messengers',
            UTMSource = 'telegram' AND UTMMedium = 'yacloudnews', 'Telegram (Yandex.Cloud)',
            UTMSource IN ('bno','koldunzhik','objectanswer','teasermain','yandex','yaserp','ymainteaser'), 'Yandex Portal',
            UTMSource IN ('dialogsblog','habr','habr','habr','habrahabr','referrals','Searchengines.ru','tech.yandex.ru','techyandexru','techyandexru','techyandexru','vc','wmblog','yametrika','yametrika','ywmmenu', 'tproger_ru', 'zen', 'techblog', 'ywebmasterblog', 'sender', 'r-point', 'retail_conf'), 'Referrals',
            UTMSource IN ('emailing', 'emailings', 'emailing^'), 'Emailing',
            UTMSource IN ('console', 'banner-console', 'scale2019', 'console-en', 'cloud.yandex', 'banner-site', 'blog-site-link', 'fb_from_scale2019', 'tg_from_scale2019', 'vk_from_scale2019', 'tw_from_scale2019'), 'YCloud Site',
            UTMMedium != '' OR UTMSource != '', 'Performance',
            TraficSourceID == 3, 'Performance',
            TraficSourceID == 5, 'Unknown',
            TraficSourceID == 2, 'Organic Search',
            TraficSourceID == 8, 'Social (organic)',
            TraficSourceID == 1, 'Referrals',
            TraficSourceID == 0, 'Direct',
            TraficSourceID == -1, 'AutoReferrals',
            TraficSourceID == 4, 'Direct',
            TraficSourceID == 9, 'Recommender Systems',
            TraficSourceID == 10, 'Messengers',
            'Other'
        ) as channel_detailed,
        AdBlock as ad_block,
        ResolutionDepth as resolution_depth,
        IsRobot as is_robot,
        StartURL as start_url
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '2020-05-01'
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
        AND StartDate >= '2020-05-01'
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
        Interests as general_interests,
        FirstVisit as first_visit_dt,
        'visit' as event
    FROM
        visits_all
    WHERE
        counter_id IN (50027884, 48570998)
        AND StartDate >= '2020-05-01'
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
            AND StartDate >= '2020-05-01'
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
            AND EventDate >= '2020-05-01'
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
            TraficSourceID == 3, 'performance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            TraficSourceID == 9, 'recommender systems',
            TraficSourceID == 10, 'messengers',
            'unknown'
        ) as utm_source,
        multiIf(
            UTMMedium IS NOT NULL AND UTMMedium != '', UTMMedium,
            TraficSourceID == 3, 'performance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            TraficSourceID == 9, 'recommender systems',
            TraficSourceID == 10, 'messengers',
            'unknown'
        ) as utm_medium,
        multiIf(
            UTMCampaign IS NOT NULL AND UTMCampaign != '', UTMCampaign,
            TraficSourceID == 3, 'performance',
            TraficSourceID == 5, 'unknown',
            TraficSourceID == 2, 'organic search',
            TraficSourceID == 8, 'social',
            TraficSourceID == 1, 'referrals',
            TraficSourceID == 0, 'direct',
            TraficSourceID == -1, 'autoreferrals',
            TraficSourceID == 4, 'direct',
            TraficSourceID == 9, 'recommender systems',
            TraficSourceID == 10, 'messengers',
            'unknown'
        ) as utm_campaign,
        UTMTerm as utm_term,
        UTMContent as utm_content,
        SearchPhrase as search_phrase,
        multiIf(
            UTMSource IN ('amp_eskimobi', 'exs', 'share2'), 'Unknown',
            UTMSource = 'koldunshik' AND UTMMedium = 'cpc', 'Yandex Portal',
            UTMMedium IN ('cpc', 'paidpost', 'cpm'), 'Performance',
            UTMSource IN ('facebook','fb','tw','vk', 'ok', 'twitter.com', 'facebook.com', 'vk.com'), 'Social (organic)',
            UTMSource IN ('t.me/ppc_analytics','telegram','telergam','tg', 'telegram.me', 'telegram.me'), 'Messengers',
            UTMSource = 'telegram' AND UTMMedium = 'yacloudnews', 'Telegram (Yandex.Cloud)',
            UTMSource IN ('bno','koldunzhik','objectanswer','teasermain','yandex','yaserp','ymainteaser'), 'Yandex Portal',
            UTMSource IN ('dialogsblog','habr','habr','habr','habrahabr','referrals','Searchengines.ru','tech.yandex.ru','techyandexru','techyandexru','techyandexru','vc','wmblog','yametrika','yametrika','ywmmenu', 'tproger_ru', 'zen', 'techblog', 'ywebmasterblog', 'sender', 'r-point', 'retail_conf'), 'Referrals',
            UTMSource IN ('emailing', 'emailings', 'emailing^'), 'Emailing',
            UTMSource IN ('console', 'banner-console', 'scale2019', 'console-en', 'cloud.yandex', 'banner-site', 'blog-site-link', 'fb_from_scale2019', 'tg_from_scale2019', 'vk_from_scale2019', 'tw_from_scale2019'), 'YCloud Site',
            UTMMedium != '' OR UTMSource != '', 'Performance',
            TraficSourceID == 3, 'Performance',
            TraficSourceID == 5, 'Unknown',
            TraficSourceID == 2, 'Organic Search',
            TraficSourceID == 8, 'Social (organic)',
            TraficSourceID == 1, 'Referrals',
            TraficSourceID == 0, 'Direct',
            TraficSourceID == -1, 'AutoReferrals',
            TraficSourceID == 4, 'Direct',
            TraficSourceID == 9, 'Recommender Systems',
            TraficSourceID == 10, 'Messengers',
            'Other'
        ) as channel,
        multiIf(
            UTMSource IN ('amp_eskimobi', 'exs', 'share2'), 'Unknown',
            UTMSource = 'koldunshik' AND UTMMedium = 'cpc', 'Yandex Portal',
            UTMMedium IN ('cpc', 'paidpost', 'cpm'), 'Performance',
            UTMSource IN ('facebook','fb','tw','vk', 'ok', 'twitter.com', 'facebook.com', 'vk.com'), 'Social (organic)',
            UTMSource IN ('t.me/ppc_analytics','telegram','telergam','tg', 'telegram.me', 'telegram.me'), 'Messengers',
            UTMSource = 'telegram' AND UTMMedium = 'yacloudnews', 'Telegram (Yandex.Cloud)',
            UTMSource IN ('bno','koldunzhik','objectanswer','teasermain','yandex','yaserp','ymainteaser'), 'Yandex Portal',
            UTMSource IN ('dialogsblog','habr','habr','habr','habrahabr','referrals','Searchengines.ru','tech.yandex.ru','techyandexru','techyandexru','techyandexru','vc','wmblog','yametrika','yametrika','ywmmenu', 'tproger_ru', 'zen', 'techblog', 'ywebmasterblog', 'sender', 'r-point', 'retail_conf'), 'Referrals',
            UTMSource IN ('emailing', 'emailings', 'emailing^'), 'Emailing',
            UTMSource IN ('console', 'banner-console', 'scale2019', 'console-en', 'cloud.yandex', 'banner-site', 'blog-site-link', 'fb_from_scale2019', 'tg_from_scale2019', 'vk_from_scale2019', 'tw_from_scale2019'), 'YCloud Site',
            UTMMedium != '' OR UTMSource != '', 'Performance',
            TraficSourceID == 3, 'Performance',
            TraficSourceID == 5, 'Unknown',
            TraficSourceID == 2, 'Organic Search',
            TraficSourceID == 8, 'Social (organic)',
            TraficSourceID == 1, 'Referrals',
            TraficSourceID == 0, 'Direct',
            TraficSourceID == -1, 'AutoReferrals',
            TraficSourceID == 4, 'Direct',
            TraficSourceID == 9, 'Recommender Systems',
            TraficSourceID == 10, 'Messengers',
            'Other'
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
        Interests as general_interests,
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
            AND StartDate >= '{0}'
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
            AND EventDate >= '{0}'
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
