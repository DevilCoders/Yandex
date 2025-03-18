#!/usr/bin/env python


from .utils import ALL_YANDEX_ZONES, forward, permanent, redirect
from .minotaur2451 import gen_minotaur2451
from .minotaur3120 import gen_minotaur3120


def gen_maps_metro():
    res = []
    # MINOTAUR-2416
    for h in [str('maps'), str('metro')]:
        for tld in [str('by'), str('com'), str('com.tr'), str('kz'), str('ru'), str('ua')]:
            maps_static_prefix = 'yastatic.net/s3/front-maps-static/maps-front-redirect/src/static/'
            maps_h_tld = {'h': h, 'tld': tld}
            res += [
                permanent(
                    '%(h)s.yandex.%(tld)s/*' % maps_h_tld,
                    'yandex.%(tld)s/%(h)s/{path}{query}' % maps_h_tld
                ),
                forward(
                    '%(h)s.yandex.%(tld)s/.well-known/assetlinks.json' % maps_h_tld,
                    maps_static_prefix + h + '/assetlinks.json'
                ),
                forward(
                    '%(h)s.yandex.%(tld)s/.well-known/apple-app-site-association' % maps_h_tld,
                    maps_static_prefix + h + '/apple-app-site-association.json'
                ),
            ]
    return res


def gen_travel():
    # MINOTAUR-2449, MINOTAUR-2528
    res = []
    for h in [
        str('train.yandex.ru'),
        str('trains.yandex.by'),
        str('trains.yandex.kz'),
        str('trains.yandex.ru'),
        str('trains.yandex.ua'),
        str('trains.yandex.uz'),
    ]:
        res += [
            permanent(h + '/*', 'travel.yandex.ru/trains/{path}{query}'),
            permanent(h + '/robots.txt', 'travel.yandex.ru/robots.txt'),
        ]

    # MINOTAUR-2474, MINOTAUR-2528
    res += [
        permanent('mir.trains.yandex.ru/*', 'travel.yandex.ru/trains{query}'),
        forward('mir.trains.yandex.ru/robots.txt', 'yastatic.net/s3/travel/other-projects/mir/robots.txt'),
    ]

    # MINOTAUR-2315, MINOTAUR-2554, MINOTAUR-3051
    for h in [
        str('tourism.yandex.ru'),
        str('travel.yandex'),
        str('travel.yandex.net'),
        str('turism.yandex.ru'),
        str('turizm.yandex.ru'),
        str('yandex.travel'),
        str('m.travel.yandex.ru'),
    ]:
        res += [
            permanent(h + '/*', 'travel.yandex.ru/{path}{query}'),
            forward(h + '/robots.txt', 'yastatic.net/s3/travel/static/_/robots/disallow.txt'),
        ]

    for h in [
        str('tury.yandex.ru'),
        str('tours.yandex.ru'),
    ]:
        res += [
            permanent(h + '/*', 'https://travel.yandex.ru/tours/{path}{query}'),
            forward(h + '/robots.txt', 'yastatic.net/s3/travel/static/_/robots/disallow.txt'),
        ]

    res += [
        permanent('hotels.yandex.ru/*', 'https://travel.yandex.ru/hotels/{path}{query}'),
        forward('hotels.yandex.ru/robots.txt', 'yastatic.net/s3/travel/static/_/robots/disallow.txt'),
    ]

    # MINOTAUR-2802
    res += [forward('travel.yandex.net/static/html/uxfeedback.html', 'yastatic.net/s3/travel/static/_/html/uxfeedback.html')]

    # MINOTAUR-2513, no certs needed
    for d in [
        str('hotelyandex.ru'), str('hotel-yandex.ru'), str('hotels-yandex.ru'), str('hotelsyandex.ru'),
        str('yandex-hotels.ru'), str('yandexhotel.ru'), str('yandex-hotel.ru'), str('busyandex.ru'),
        str('bus-yandex.ru'), str('yandex-bus.ru'), str('yandexbus.ru'), str('yandexhotels.kz'),
    ]:
        res += [permanent('%s/*' % d, 'travel.yandex.ru/')]

    return res


def gen_tutor():
    res = []
    # MINOTAUR-2468
    for h in [
        str('ege.ya.ru'),
        str('eg.yandex.ru'),
        str('www.eg.yandex.ru'),
        str('ege.yandex.ru'),
        str('old.ege.yandex.ru'),
        str('www.ege.yandex.ru'),
    ]:
        res += [
            permanent(h + '/', 'yandex.ru/tutor/'),

            permanent(h + '/ege', 'yandex.ru/tutor/'),
            permanent(h + '/ege/mathematics', 'yandex.ru/tutor/subject/?subject_id=2'),
            permanent(h + '/ege/russian', 'yandex.ru/tutor/subject/?subject_id=3'),
            permanent(h + '/ege/physics', 'yandex.ru/tutor/subject/?subject_id=4'),
            permanent(h + '/ege/literature', 'yandex.ru/tutor/subject/?subject_id=5'),
            permanent(h + '/ege/informatics', 'yandex.ru/tutor/subject/?subject_id=6'),
            permanent(h + '/ege/chemistry', 'yandex.ru/tutor/subject/?subject_id=7'),
            permanent(h + '/ege/biology', 'yandex.ru/tutor/subject/?subject_id=8'),
            permanent(h + '/ege/geography', 'yandex.ru/tutor/subject/?subject_id=9'),
            permanent(h + '/ege/history', 'yandex.ru/tutor/subject/?subject_id=10'),
            permanent(h + '/ege/social', 'yandex.ru/tutor/subject/?subject_id=11'),
            permanent(h + '/ege/english', 'yandex.ru/tutor/subject/?subject_id=12'),
            permanent(h + '/ege/german', 'yandex.ru/tutor/subject/?subject_id=13'),
            permanent(h + '/ege/spanish', 'yandex.ru/tutor/subject/?subject_id=14'),
            permanent(h + '/ege/french', 'yandex.ru/tutor/subject/?subject_id=15'),

            permanent(h + '/oge', 'yandex.ru/tutor/?exam_id=2'),
            permanent(h + '/oge/mathematics', 'yandex.ru/tutor/subject/?subject_id=16'),
            permanent(h + '/oge/russian', 'yandex.ru/tutor/subject/?subject_id=17'),
            permanent(h + '/oge/physics', 'yandex.ru/tutor/subject/?subject_id=18'),
            permanent(h + '/oge/chemistry', 'yandex.ru/tutor/subject/?subject_id=21'),
            permanent(h + '/oge/biology', 'yandex.ru/tutor/subject/?subject_id=22'),
            permanent(h + '/oge/geography', 'yandex.ru/tutor/subject/?subject_id=23'),
            permanent(h + '/oge/social', 'yandex.ru/tutor/subject/?subject_id=25'),
        ]
    return res


def gen_redirects():
    res = []

    res += gen_maps_metro()
    res += gen_travel()
    res += gen_tutor()

    # MINOTAUR-3123
    for tld in ALL_YANDEX_ZONES:
        if tld in [str('com'), str('com.tr')]:
            res += [permanent('yandex.%s/favicon.ico' % tld, 'yastatic.net/lego/_/rBTjd6UOPk5913OSn5ZQVYMTQWQ.ico')]
        else:
            res += [permanent('yandex.%s/favicon.ico' % tld, 'yastatic.net/lego/_/pDu9OWAQKB0s2J9IojKpiS_Eho.ico')]

    # MINOTAUR-2476
    for tld in [str('ru'), str('by'), str('kz'), str('ua'), str('com')]:
        res += [permanent('www.afisha.yandex.%s/*' % tld, 'afisha.yandex.%s/{path}{query}' % tld)]

    # MINOTAUR-2493
    for d in [
        str('clickhouse.yandex'), str('yandex.ru/clickhouse'), str('yandex.com/clickhouse'),
        str('clickhouse.yandex.ru'), str('clickhouse.yandex.com'),
    ]:
        res += [permanent('%s/*' % d, 'clickhouse.tech/{path}{query}')]

    # MINOTAUR-2512
    for tld in [str('ru'), str('by'), str('ua'), str('kz'), str('com'), str('com.tr')]:
        res += [permanent('wdgt.yandex.%s/*' % tld, 'yandex.%s/' % tld)]

    # MINOTAUR-2523, MINOTAUR-2563
    for tld in [str('ru'), str('com'), str('com.tr')]:
        res += [
            permanent('tech.yandex.%s/*' % tld, 'yandex.%s/dev/{path}{query}' % tld),
            permanent('api.yandex.%s/*' % tld, 'yandex.%s/dev/{path}{query}' % tld),
        ]

    # MINOTAUR-2526
    res += [permanent('auto.yandex.ru/*', 'auto.yandex/{path}{query}')]

    # MINOTAUR-2533, MINOTAUR-2542
    res += [
        permanent('pogoda.yandex.ru/*', 'yandex.ru/pogoda/{path}{query}'),
        permanent('pogoda.yandex.com/*', 'yandex.com/weather/{path}{query}'),
        permanent('pogoda.yandex.ua/*', 'yandex.ua/pogoda/{path}{query}'),
        permanent('hava.yandex.com.tr/*', 'yandex.com.tr/hava/{path}{query}'),
        permanent('pogoda.yandex.by/*', 'yandex.by/pogoda/{path}{query}'),
        permanent('weather.yandex.ru/*', 'yandex.ru/pogoda/{path}{query}'),
        permanent('pogoda.yandex.kz/*', 'yandex.kz/pogoda/{path}{query}'),
        permanent('m.pogoda.yandex.ru/*', 'yandex.ru/pogoda/{path}{query}'),
        permanent('mini.pogoda.yandex.ru/*', 'p.ya.ru/{path}{query}'),
        permanent('pogoda.ya.ru/*', 'yandex.ru/pogoda/{path}{query}'),
        permanent('weather.ya.ru/*', 'yandex.ru/pogoda/{path}{query}'),
    ]

    # MINOTAUR-2537, MINOTAUR-2543
    for d in [str('123.yandex'), str('123.ya.ru')]:
        res += [
            permanent('%s/' % d, 'education.yandex.ru/'),
            permanent('%s/*' % d, 'education.yandex.ru/{path}{query}')
        ]

    # MINOTAUR-2541
    res += [
        permanent('phone-passport.yandex.ru/', 'passport.yandex.ru/profile/phones'),
        permanent('phone-passport.yandex.com/', 'passport.yandex.com/profile/phones'),
        permanent('phone-passport.yandex.com.tr/', 'passport.yandex.tr/profile/phones'),
    ]

    # MINOTAUR-2548
    for tld in [str('com'), str('com.tr'), str('by'), str('ua'), str('kz')]:
        res += [permanent('yandex.%s/ie/*' % tld, 'yandex.ru/ie/{path}{query}')]

    # MINOTAUR-2571
    for tld in [str('fi'), str('pl')]:
        res += [
            redirect('yandex.%s/*' % tld, 'yandex.eu/{path}{query}'),
            redirect('www.yandex.%s/*' % tld, 'yandex.eu/{path}{query}'),
            redirect('l7test.yandex.%s/*' % tld, 'l7test.yandex.eu/{path}{query}'),
        ]

    # MINOTAUR-2584, MINOTAUR-2621
    for h in [
        str('drive.yandex'),
        str('www.drive.yandex'),
        str('drive.yandex.ru'),
        str('drive.ya.ru'),
        str('carsharing.yandex.ru'),
        str('drive.yandex.com'),
    ]:
        res.append(redirect('%s/*' % h, 'yandex.ru/drive', 303))

    # MINOTAUR-2600
    for h in [str('phone.yandex.ru'), str('phone.yandex.com')]:
        res.append(permanent(h + '/*', 'market.yandex.ru/product--smartfon-yandex-telefon/177547282{query}'))

    # MINOTAUR-2451
    res += gen_minotaur2451()

    # BALANCERSUPPORT-1245
    res += [permanent('yandex.ru/o/*', 'o.yandex.ru/{path}{query}')]

    # MINOTAUR-2598
    res += [permanent('market.ru/*', 'market.yandex.ru/{path}{query}')]

    # MINOTAUR-2609
    res += [permanent('yandex.ru/blog/toloka/*', 'https://toloka.ai/ru/blog/{path}{query}')]

    # MINOTAUR-2615
    res += [permanent('o.ya.ru/*', 'o.yandex.ru/{path}{query}')]

    # MINOTAUR-2642
    res += [permanent('dialog.yandex.ru/*', 'dialogs.yandex.ru/{path}{query}')]

    # MINOTAUR-2641
    for tld in [str('ru'), str('com.tr'), str('ua')]:
        res += [permanent('home.yandex.%s/*' % tld, 'yandex.%s/soft/home/{path}{query}' % tld)]

    # MINOTAUR-2640
    for tld in [str('by'), str('com'), str('kz')]:
        res += [permanent('www.business.taxi.yandex.%s/*' % tld, 'https://business.taxi.yandex.%s/{path}' % tld)]

    res += [permanent('www.business.yango.yandex.com/*', 'https://business.yango.yandex.com/{path}')]
    res += [permanent('www.go.ya.ru/*', 'https://go.ya.ru/{path}')]
    res += [permanent('www.go.yandex/*', 'https://go.yandex/{path}')]

    for tld in [str('by'), str('com'), str('com.ge'), str('ee'), str('kg'), str('kz'), str('lt'), str('lv'), str('md'), str('net'), str('rs'), str('ru'), str('uz')]:
        res += [permanent('www.go.yandex.%s/*' % tld, 'https://go.yandex.%s/{path}' % tld)]

    res += [permanent('www.support-uber.com/*', 'https://support-uber.com/{path}')]
    res += [permanent('www.taxi.ya.ru/*', 'https://taxi.ya.ru/{path}')]

    for tld in [str('by'), str('com'), str('com.ge'), str('ee'), str('kg'), str('kz'), str('lt'), str('lv'), str('md'), str('rs'), str('ru'), str('ua'), str('uz')]:
        res += [permanent('www.taxi.yandex.%s/*' % tld, 'https://taxi.yandex.%s/{path}' % tld)]

    res += [permanent('www.yandex.taxi/*', 'https://yandex.taxi/{path}')]
    res += [permanent('www.yango.yandex.com/*', 'https://yango.yandex.com/{path}')]

    # MINOTAUR-2650
    res += [permanent('www.agg.taxi.yandex.net/*', 'https://agg.taxi.yandex.net/{path}')]
    res += [permanent('www.agg.taximeter.yandex.ru/*', 'https://agg.taximeter.yandex.ru/{path}')]
    res += [permanent('www.aggregator.taxi.yandex.net/*', 'https://aggregator.taxi.yandex.net/{path}')]

    # MINOTAUR-2647
    res += [permanent('realty.ya.ru/*', 'https://realty.yandex.ru/{path}{query}')]

    # MINOTAUR-2664
    for tld in [str('ru'), str('by'), str('uz'), str('kz'), str('ua')]:
        res += [
            permanent('local.yandex.%s/*' % tld, 'https://yandex.%s' % tld),
            permanent('yandex.%s/local/*' % tld, 'https://yandex.%s' % tld),
            forward('local.yandex.%s/robots.txt' % tld, 'yastatic.net/s3/district/robots.txt'),
            forward('local.yandex.%s/.well-known/assetlinks.json' % tld, 'yastatic.net/s3/district/.well-known/assetlinks.json')
        ]

    # MINOTAUR-2679
    res += [permanent('seller.yandex.ru/*', 'https://partner.market.yandex.ru/{path}{query}')]

    # MINOTAUR-2703
    res += [
        permanent('wiki-eva-test.yandex.ru/users/*', 'https://wiki.test.yandex-team.ru/users/{path}{query}'),
        permanent('wiki-eva-test.yandex.ru/*', 'https://wiki.test.yandex-team.ru/eva/{path}{query}'),

        permanent('wiki-front.eva.yandex-team.ru/users/*', 'https://wiki.yandex-team.ru/users/{path}{query}'),
        permanent('wiki.eva.yandex-team.ru/users/*', 'https://wiki.yandex-team.ru/users/{path}{query}'),

        permanent('wiki-front.eva.yandex-team.ru/*', 'https://wiki.yandex-team.ru/eva/{path}{query}'),
        permanent('wiki.eva.yandex-team.ru/*', 'https://wiki.yandex-team.ru/eva/{path}{query}')
    ]

    # MINOTAUR-2639
    res += [
        permanent('harita.yandex.com.tr/*', 'https://yandex.com.tr/harita/{path}{query}'),
        forward('harita.yandex.com.tr/.well-known/assetlinks.json',
                'yastatic.net/s3/front-maps-static/maps-front-redirect/src/static/maps/assetlinks.json'),
        forward('harita.yandex.com.tr/.well-known/apple-app-site-association',
                'yastatic.net/s3/front-maps-static/maps-front-redirect/src/static/maps/apple-app-site-association.json')
    ]

    # MINOTAUR-2632
    for tld in [str('by'), str('com.ua'), str('kz')]:
        res += [permanent('dns.yandex.%s/*' % tld, 'dns.yandex.ru/{path}{query}')]

    # MINOTAUR-2740
    res += [
        permanent('www.dostavka.yandex.ru/*', 'https://dostavka.yandex.ru/{path}{query}'),
        permanent('www.delivery.yandex.com/*', 'https://delivery.yandex.com/{path}{query}')
    ]

    # MINOTAUR-2718 MINOTAUR-2784 MINOTAUR-2876
    for tld in [str('com'), str('com.tr'), str('az'), str('co.il'), str('com.am'), str('com.ge'), str('ee'), str('fr'),
                str('kg'), str('lt'), str('lv'), str('md'), str('tj'), str('tm')]:
        res += [forward('yandex.%s/ads.txt' % tld, 'yastatic.net/s3/games-static/static-data/ads.txt')]
        res += [forward('yandex.%s/app-ads.txt' % tld, 'yastatic.net/s3/games-static/static-data/app-ads.txt')]

    # MINOTAUR-2835 MINOTAUR-2876
    for tld in [str('ru'), str('by'), str('kz'), str('uz'), str('ua')]:
        res += [forward('yandex.%s/ads.txt' % tld, 'yastatic.net/s3/games-static/static-data/ru-ads.txt')]
        res += [forward('yandex.%s/app-ads.txt' % tld, 'yastatic.net/s3/games-static/static-data/app-ads.txt')]

    # MINOTAUR-3110 MINOTAUR-3177
    for tld in ALL_YANDEX_ZONES:
        res += [forward('yandex.%s/robots.txt' % tld, 'yastatic.net/s3/robots-files/robots_%s.txt' % tld)]
        res += [forward('yandex.%s/apple-touch-icon-precomposed.png' % tld, 'yastatic.net/s3/web4static/_/v2/I2UdfRGGrwcbCB0Z6uoCAftI7z8.png')]

    # MINOTAUR-3117
    for port in ['80', '443']:
        for tld in ALL_YANDEX_ZONES:
            res += [forward('yandex.%s:%s/robots.txt' % (tld, port), 'yastatic.net/s3/robots-files/robots_ports.txt')]

    # MINOTAUR-2757
    res += [permanent('job.yandex.ru/*', 'https://yandex.ru/jobs')]

    # MINOTAUR-2763
    res += [
        permanent('avtochel.ru/*', 'https://auto.ru/{path}{query}'),
        permanent('avtoru.tv/*', 'https://auto.ru/{path}{query}'),
        permanent('autoru.tv/*', 'https://auto.ru/{path}{query}')
    ]

    # MINOTAUR-2773
    res += [permanent('care.yandex/*', 'https://help.yandex.ru')]

    # MINOTAUR-2774
    res += [permanent('beta.dialogs.yandex.ru/*', 'https://dialogs.yandex.ru/{path}{query}')]

    # MINOTAUR-2771
    res += [redirect('store.yandex/*', 'https://market.yandex.ru/business--yandex/891400')]

    # MINOTAUR-2785
    res += [permanent('h.ya.ru/*', 'https://health.yandex.ru/{path}{query}')]

    # MINOTAUR-2804
    res += [
        permanent('images.yandex.com/*', 'https://yandex.com/images/{path}{query}'),
        permanent('video.yandex.com/*', 'https://yandex.com/video/{path}{query}')
    ]

    # MINOTAUR-2834
    res += [permanent('beta.wiki.yandex-team.ru/*', 'https://wiki.yandex-team.ru/{path}{query}')]

    # MINOTAUR-2840
    res += [permanent('xn--80ae0biii.xn--p1ai/*', 'https://auto.ru/{path}{query}')]

    # MINOTAUR-2850
    for tld in [str('com'), str('ru')]:
        res += [permanent('thequestion.%s/*' % tld, 'https://yandex.ru/q/{path}')]

    # BALANCERSUPPORT-3190
    res += [permanent('yandex.ru/zenpromo/*', 'https://yandex.ru/{path}{query}')]

    # MINOTAUR-2870
    res.append(redirect('go.yandex.net/*', 'https://go.yandex/ru_ru/{path}'))

    # MINOTAUR-2877
    for d in [
        'school.yandex.ru',
        'sections.yandex.ru',
        'initiative.yandex.ru'
    ]:
        res += [permanent(d + '/*', 'fund.yandex.ru/{query}')]

    # MINOTAUR-2880
    res += [permanent('praktikum.yandex.ru/*', 'https://practicum.yandex.ru/{path}{query}')]
    res += [permanent('praktikum.yandex.com/*', 'https://practicum.yandex.com/{path}{query}')]

    # MINOTAUR-2883
    for d in [
        str('disk.yandex'),
        str('docs.yandex'),
        str('telemost.yandex')
    ]:
        res += [permanent(d + '/*', '%s.ru/{path}{query}' % d)]

    # MINOTAUR-2905
    res += [permanent('plus.ya.ru/*', 'https://plus.yandex.ru/{path}{query}')]

    # MINOTAUR-2920
    for d in [
        str('efir/*'),
        str('portal/video/*'),
        str('portal/tvstream/*'),
        str('portal/tvstream_json/channels'),
    ]:
        res += [redirect('https://yandex.ru/%s' % d, 'https://yastatic.net/s3/zen-misc/ether/ether.html')]

    # MINOTAUR-2928
    res += [redirect('wearemagiccamp.com/cardboardmedia', 'https://market.yandex.ru/cardboardmedia')]

    # MINOTAUR-2944
    # MINOTAUR-2985
    for tld in [
        'ru', 'az', 'by', 'co.il', 'com',
        'com.am', 'com.ge', 'com.tr', 'ee', 'eu',
        'fi', 'fr', 'kg', 'kz', 'lt',
        'lv', 'md', 'pl', 'tj', 'tm',
        'ua', 'uz',
    ]:
        for p in [
            'apteki*',
            'apteki/search',
            'apteki/cart',
            'apteki/order',
            'apteki/new-map',
            'articles',
            'pills*',
        ]:
            res += [permanent('yandex.%s/health/%s' % (tld, p), 'https://market.yandex.ru/catalog/72412?from=apteki')]
        res += [permanent('yandex.%s/health/apteki/product/*' % tld, 'https://market.yandex.ru/pharmaoffer/{path}{query}')]

    # MINOTAUR-2969
    res += [permanent('cms.tickets.yandex.ru/*', 'https://cms.tickets.yandex.net/{path}{query}')]

    # MINOTAUR-3091
    res += [permanent('commo.ru/*', 'https://market.yandex.ru/promo/commo/{path}{query}')]
    res += [permanent('www.commo.ru/*', 'https://market.yandex.ru/promo/commo/{path}{query}')]

    # MINOTAUR-2960
    res += [redirect('taxi.yandex/*', 'https://taxi.yandex.ru')]

    # MINOTAUR-3120
    res += gen_minotaur3120()

    # notanymore.yandex.ru - pay your respects and leave this redirect at the end of the function
    # MINOTAUR-2622 MINOTAUR-2766 MINOTAUR-2805 MINOTAUR-2806 MINOTAUR-2836 MINOTAUR-2837 MINOTAUR-3133
    for d in [
        str('scantobuy.ru'),
        str('supercheck.ru'),
        str('bookalook.ru'),
        str('spbsoftware.com'),
        str('spb.com'),
        str('xn--80accmwzdqdad2jf.xn--p1ai'),
        str('xn--80adgenvofc1a.xn--p1ai'),
        str('agnitum.com'),
        str('www.agnitum.com'),
    ]:
        res += [permanent(d + '/*', 'notanymore.yandex.ru')]

    return res
