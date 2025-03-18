#!/usr/bin/env python


from .utils import permanent, rewrite_xml


def gen_minotaur2451():
    res = []

    for tld in [
        str('ru'),
        str('by'),
        str('kz'),
        str('ua'),
        str('com'),
    ]:
        for h in [
            'advertising.yandex.%s' % tld,
            'www.advertising.yandex.%s' % tld,
            'adv.yandex.%s' % tld,
            'www.adv.yandex.%s' % tld,
        ]:
            res += [
                permanent(h + '/*', 'yandex.%s' % tld + '/adv/{path}')
            ]

    for h in [
        str('advertising.yandex.com.tr'),
        str('www.advertising.yandex.com.tr'),
    ]:
        res += [
            permanent(h + '/*', 'yandex.com.tr/adv/{path}')
        ]

    for h in [
        str('adv.yandex.ru'),
        str('advertising.yandex.ru'),
        str('www.adv.yandex.ru'),
        str('www.advertising.yandex.ru'),
    ]:
        res += [
            # ^/news/(.*)/?$ - https://yandex.ru/adv/news
            permanent(h + '/news/*', 'yandex.ru/adv/news'),

            # ^/price/?$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price', 'yandex.ru/adv/prices-common'),

            # ^/price\.xml$ - https://yandex.ru/adv/prices
            permanent(h + '/price.xml', 'yandex.ru/adv/prices'),

            # ^/kupislova\.xml$ - https://yandex.ru/adv/
            permanent(h + '/kupislova.xml', 'yandex.ru/adv/'),

            # ^/price/context/market\.xml$ - https://yandex.ru/adv/products/classified/market#price
            permanent(h + '/price/context/market.xml', 'yandex.ru/adv/products/classified/market#price'),

            # ^/price/discount/context\.xml$ - https://yandex.ru/adv/discount-context
            permanent(h + '/price/discount/context.xml', 'yandex.ru/adv/discount-context'),

            # ^/price/priority/sprav\.xml$ - https://yandex.ru/adv/products/geo/spravprice
            permanent(h + '/price/priority/sprav.xml', 'yandex.ru/adv/products/geo/spravprice'),

            # ^/price/other/catalog\.xml$ - https://yandex.ru/adv/products/classified/catalogue#price
            permanent(h + '/price/other/catalog.xml', 'yandex.ru/adv/products/classified/catalogue#price'),

            # ^/price/media/maps_autoru\.xml$ - https://yandex.ru/adv/products/display/maps-autoru#price
            permanent(h + '/price/media/maps_autoru.xml', 'yandex.ru/adv/products/display/maps-autoru#price'),

            # ^/price/media/market\.xml$ - https://yandex.ru/adv/products/display/marketmedia#price
            permanent(h + '/price/media/market.xml', 'yandex.ru/adv/products/display/marketmedia#price'),

            # ^/price/media/kinopoisk\.xml$ - https://yandex.ru/adv/products/display/kinopoiskmedia#price
            permanent(h + '/price/media/kinopoisk.xml', '/adv/products/display/kinopoiskmedia#price'),

            # ^/price/media/videoweb_yandex\.xml$ - https://yandex.ru/adv/products/display/videoweb#price
            permanent(h + '/price/media/videoweb_yandex.xml', 'yandex.ru/adv/products/display/videoweb#price'),

            # ^/price/media/videoweb_auction\.xml$ - https://yandex.ru/adv/products/display/videoweb#price
            permanent(h + '/price/media/videoweb_auction.xml', 'yandex.ru/adv/products/display/videoweb#price'),

            # ^/price/media/video\.xml$ - https://yandex.ru/adv/products/display/videobanner#price
            permanent(h + '/price/media/video.xml', 'yandex.ru/adv/products/display/videobanner#price'),

            # ^/price/media/auctionvideo\.xml$ - https://yandex.ru/adv/products/display/auction#price
            permanent(h + '/price/media/auctionvideo.xml', 'yandex.ru/adv/products/display/auction#price'),

            # ^/price/media/crossmedia_network\.xml$ - https://yandex.ru/adv/products/display/cross-media-network#price
            permanent(h + '/price/media/crossmedia_network.xml', 'yandex.ru/adv/products/display/cross-media-network#price'),

            # ^/price/media/autoru_auction\.xml$ - https://yandex.ru/adv/products/display/autoru
            permanent(h + '/price/media/autoru_auction.xml', 'yandex.ru/adv/products/display/autoru'),

            # ^/price/media/autoru\.xml$ - https://yandex.ru/adv/products/display/autoru
            permanent(h + '/price/media/autoru.xml', 'yandex.ru/adv/products/display/autoru'),

            # ^/price/discount/media\.xml$ - https://yandex.ru/adv/products/display/autoru
            permanent(h + '/price/discount/media.xml', 'yandex.ru/adv/products/display/autoru'),

            # ^/price/media/context\.xml$ - https://yandex.ru/adv/products/context/contextdisplay#price
            permanent(h + '/price/media/context.xml', 'yandex.ru/adv/products/context/contextdisplay#price'),

            # ^/price/media/pda\.xml$ - https://yandex.ru/adv/products/mobile/mobileauction#price
            permanent(h + '/price/media/pda.xml', 'yandex.ru/adv/products/mobile/mobileauction#price'),

            # ^/price/media/pda_mainpage\.xml$ - https://yandex.ru/adv/products/mobile/mobile-mainpage#price
            permanent(h + '/price/media/pda_mainpage.xml', 'yandex.ru/adv/products/mobile/mobile-mainpage#price'),

            # ^/price/media/pda_mainpage2\.xml$ - https://yandex.ru/adv/products/mobile/mobile-mainpage#price
            permanent(h + '/price/media/pda_mainpage2.xml', 'yandex.ru/adv/products/mobile/mobile-mainpage#price'),

            # ^/price/media/mobile_autoru\.xml$ - https://yandex.ru/adv/products/mobile/mobile-autoru#price
            permanent(h + '/price/media/mobile_autoru.xml', 'yandex.ru/adv/products/mobile/mobile-autoru#price'),

            # ^/price/media/auction_mobilegeo\.xml$ - https://yandex.ru/adv/products/mobile/mobileauction#price
            permanent(h + '/price/media/auction_mobilegeo.xml', 'yandex.ru/adv/products/mobile/mobileauction#price'),

            # ^/price/media/auction_mobilereach\.xml$ - https://yandex.ru/adv/products/mobile/mobileauction#price
            permanent(h + '/price/media/auction_mobilereach.xml', 'yandex.ru/adv/products/mobile/mobileauction#price'),

            # ^/price/media/auction_mobileweb\.xml$ - https://yandex.ru/adv/products/mobile/mobileauction#price
            permanent(h + '/price/media/auction_mobileweb.xml', 'yandex.ru/adv/products/mobile/mobileauction#price'),

            # ^/price/media/(uslugi|radio|page404|news|network|money|map|mail|interests|digital_radio|banmarket|audioad|geo_targeting_pack|dynamic_banner|autoru_seller_imho|autoru_seller|auto_autoru|auto)\.xml$ - https://yandex.ru/adv/prices-common
            # ^/price/media/uslugi\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/uslugi.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/radio\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/radio.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/page404\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/page404.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/news\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/news.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/network\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/network.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/money\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/money.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/map\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/map.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/mail\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/mail.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/interests\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/interests.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/digital_radio\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/digital_radio.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/banmarket\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/banmarket.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/audioad\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/audioad.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/geo_targeting_pack\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/geo_targeting_pack.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/dynamic_banner\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/dynamic_banner.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/autoru_seller_imho\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/autoru_seller_imho.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/autoru_seller\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/autoru_seller.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/auto_autoru\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/auto_autoru.xml', 'yandex.ru/adv/prices-common'),
            # ^/price/media/auto\.xml$ - https://yandex.ru/adv/prices-common
            permanent(h + '/price/media/auto.xml', 'yandex.ru/adv/prices-common'),

            # ^/price/media/(\w+)\.xml$ - https://yandex.ru/adv/products/display/$1#price
            permanent(h + '/price/media/*', 'yandex.ru/adv/products/display/{path}#price', dst_rewrites=[rewrite_xml()]),

            # ^/price/audio/radio\.xml$ - https://yandex.ru/adv/products/display/audio#price
            permanent(h + '/price/audio/radio.xml', 'yandex.ru/adv/products/display/audio#price'),

            # ^/price/audio/digital_radio\.xml$ - https://yandex.ru/adv/products/display/audio#price
            permanent(h + '/price/audio/digital_radio.xml', 'yandex.ru/adv/products/display/audio#price'),

            # ^/price/(.*) - https://yandex.ru/adv/prices-common
            permanent(h + '/price/*', 'yandex.ru/adv/prices-common'),

            # ^/requirement/media/main\.xml$ - https://yandex.ru/adv/requirements/mainpage
            permanent(h + '/requirement/media/main.xml', 'yandex.ru/adv/requirements/mainpage'),

            # ^/requirement/media/afisha\.xml$ - https://yandex.ru/adv/requirements/afishareq
            permanent(h + '/requirement/media/afisha.xml', 'yandex.ru/adv/requirements/afishareq'),

            # ^/requirement/media/videoweb\.xml$ - https://yandex.ru/adv/requirements/videowebreq
            permanent(h + '/requirement/media/videoweb.xml', 'yandex.ru/adv/requirements/videowebreq'),

            # ^/requirement/media/auction_mobileweb\.xml$ - https://yandex.ru/adv/requirements/mobileauctionreq
            permanent(h + '/requirement/media/auction_mobileweb.xml', 'yandex.ru/adv/requirements/mobileauctionreq'),

            # ^/requirement/media/(banner|regulations)\.xml$ - https://yandex.ru/legal/banner_adv_rules/
            # ^/requirement/media/banner\.xml$ - https://yandex.ru/legal/banner_adv_rules/
            permanent(h + '/requirement/media/banner.xml', 'yandex.ru/legal/banner_adv_rules/'),
            # ^/requirement/media/regulations\.xml$ - https://yandex.ru/legal/banner_adv_rules/
            permanent(h + '/requirement/media/regulations.xml', 'yandex.ru/legal/banner_adv_rules/'),

            # ^/requirement/media/general\.xml$ - https://yandex.ru/legal/adv_rules/
            permanent(h + '/requirement/media/general.xml', 'yandex.ru/legal/adv_rules/'),

            # ^/requirement/media/(vertical_banner(1)?|radio|audioad|performance_banner)\.xml$ https://yandex.ru/adv/requirements
            # ^/requirement/media/vertical_banner\.xml$ https://yandex.ru/adv/requirements
            permanent(h + '/requirement/media/vertical_banner.xml', 'yandex.ru/adv/requirements'),
            # ^/requirement/media/vertical_banner1\.xml$ https://yandex.ru/adv/requirements
            permanent(h + '/requirement/media/vertical_banner1.xml', 'yandex.ru/adv/requirements'),
            # ^/requirement/media/radio\.xml$ https://yandex.ru/adv/requirements
            permanent(h + '/requirement/media/radio.xml', 'yandex.ru/adv/requirements'),
            # ^/requirement/media/audioad\.xml$ https://yandex.ru/adv/requirements
            permanent(h + '/requirement/media/audioad.xml', 'yandex.ru/adv/requirements'),
            # ^/requirement/media/performance_banner\.xml$ https://yandex.ru/adv/requirements
            permanent(h + '/requirement/media/performance_banner.xml', 'yandex.ru/adv/requirements'),

            # ^/requirement/media/([-\w]+)\.xml$ - https://yandex.ru/adv/requirements/$1
            permanent(h + '/requirement/media/*', 'yandex.ru/adv/requirements/{path}', dst_rewrites=[rewrite_xml()]),

            # ^/requirement/audio/radio\.xml$ - https://yandex.ru/adv/requirements/audioreq
            permanent(h + '/requirement/audio/radio.xml', 'yandex.ru/adv/requirements/audioreq'),

            # ^/requirement/context/general\.xml$ - https://yandex.ru/legal/direct_adv_rules/
            permanent(h + '/requirement/context/general.xml', 'yandex.ru/legal/direct_adv_rules/'),

            # ^/requirement/context/regulations\.xml$ - https://yandex.ru/legal/direct_display_rules/
            permanent(h + '/requirement/context/regulations.xml', 'yandex.ru/legal/direct_display_rules/'),

            # ^/requirement/context/text\.xml$ - https://yandex.ru/legal/text_adv_rules/
            permanent(h + '/requirement/context/text.xml', 'yandex.ru/legal/text_adv_rules/'),

            # ^/requirement/(.*) - https://yandex.ru/adv/requirements
            permanent(h + '/requirement/*', 'yandex.ru/adv/requirements'),

            # ^/regionpartner\.xml$ - https://yandex.ru/adv/contact/agency
            permanent(h + '/regionpartner.xml', 'yandex.ru/adv/contact/agency'),

            # ^/regionagent\.xml$ - https://yandex.ru/adv/partners/
            permanent(h + '/regionagent.xml', 'yandex.ru/adv/partners/'),

            # ^/seminar/?$ - https://yandex.ru/adv/edu/events
            permanent(h + '/seminar/', 'yandex.ru/adv/edu/events'),

            # ^/seminar/index\.xml$ - https://yandex.ru/adv/edu/events
            permanent(h + '/seminar/index.xml', 'yandex.ru/adv/edu/events'),

            # ^/strategies\.xml$ - https://yandex.ru/adv/materials/strategies
            permanent(h + '/strategies.xml', 'yandex.ru/adv/materials/strategies'),

            # ^/research\.xml$ - https://yandex.ru/adv/news?tag=issledovanie
            permanent(h + '/research.xml', 'yandex.ru/adv/news?tag=issledovanie'),

            # ^/stories\.xml$ - https://yandex.ru/adv/solutions/stories
            permanent(h + '/stories.xml', 'yandex.ru/adv/solutions/stories'),

            # ^/research_article\.xml$ - https://yandex.ru/adv/news
            permanent(h + '/research_article.xml', 'yandex.ru/adv/news'),

            # ^/orderform\.xml$ - https://yandex.ru/adv/order/context/placead
            permanent(h + '/orderform.xml', 'yandex.ru/adv/order/context/placead'),

            # ^/agency/? - https://yandex.ru/adv/partners
            permanent(h + '/agency/', 'yandex.ru/adv/partners'),

            # ^/contact/?$ - https://yandex.ru/adv/contact
            permanent(h + '/contact/', 'yandex.ru/adv/contact'),

            # ^/contact/agency(/market)?/?$ - https://yandex.ru/adv/contact/agency
            # ^/contact/agency/?$ - https://yandex.ru/adv/contact/agency
            permanent(h + '/contact/agency/', 'yandex.ru/adv/contact/agency'),
            # ^/contact/agency/market/?$ - https://yandex.ru/adv/contact/agency
            permanent(h + '/contact/agency/market/', 'yandex.ru/adv/contact/agency'),

            # ^/contact/(.*) - https://yandex.ru/adv/contact/
            permanent(h + '/contact/*', 'yandex.ru/adv/contact/'),

            # ^/promocodes\.xml$ - https://yandex.ru/adv/products/actions
            permanent(h + '/promocodes.xml', 'yandex.ru/adv/products/actions'),

            # ^/advertiser/education/(outer|partners)\.xml$ - https://yandex.ru/adv/edu/$1
            # ^/advertiser/education/outer\.xml$ - https://yandex.ru/adv/edu/outer
            permanent(h + '/advertiser/education/outer.xml', 'yandex.ru/adv/edu/outer'),
            # ^/advertiser/education/partners\.xml$ - https://yandex.ru/adv/edu/partners
            permanent(h + '/advertiser/education/partners.xml', 'yandex.ru/adv/edu/partners'),

            # ^/advertiser/? - https://yandex.ru/adv/edu
            permanent(h + '/advertiser/', 'yandex.ru/adv/edu'),

            # ^/context/direct/platforms\.xml$ - https://yandex.ru/adv/products/materials/yan
            permanent(h + '/context/direct/platforms.xml', 'yandex.ru/adv/products/materials/yan'),

            # ^/context/sprav/? - https://yandex.ru/adv/products/geo
            permanent(h + '/context/sprav/', 'yandex.ru/adv/products/geo'),

            # ^/context/market/? - https://yandex.ru/adv/products/classified/market
            permanent(h + '/context/market/', 'yandex.ru/adv/products/classified/market'),

            # ^/context/? - https://yandex.ru/adv/products/context
            permanent(h + '/context/', 'yandex.ru/adv/products/context'),

            # ^/audio/radio\.xml$ - https://yandex.ru/adv/products/display/audio
            permanent(h + '/audio/radio.xml', 'yandex.ru/adv/products/display/audio'),

            # ^/media/banner/(pda|pda_mainpage_tab)\.xml$ - https://yandex.ru/adv/products/mobile
            # ^/media/banner/pda\.xml$ - https://yandex.ru/adv/products/mobile
            permanent(h + '/media/banner/pda.xml', 'yandex.ru/adv/products/mobile'),
            # ^/media/banner/pda_mainpage_tab\.xml$ - https://yandex.ru/adv/products/mobile
            permanent(h + '/media/banner/pda_mainpage_tab.xml', 'yandex.ru/adv/products/mobile'),

            # ^/media/banner/pda_mainpage\.xml$ - https://yandex.ru/adv/products/mobile/mobile-mainpage
            permanent(h + '/media/banner/pda_mainpage.xml', 'yandex.ru/adv/products/mobile/mobile-mainpage'),

            # ^/media/banner/mobile_autoru\.xml$ - https://yandex.ru/adv/products/mobile/mobile-autoru
            permanent(h + '/media/banner/mobile_autoru.xml', 'yandex.ru/adv/products/mobile/mobile-autoru'),

            # ^/media/banner/mobile_auction\.xml$ - https://yandex.ru/adv/products/mobile/mobileauction
            permanent(h + '/media/banner/mobile_auction.xml', 'yandex.ru/adv/products/mobile/mobileauction'),

            # ^/media/banner/maps_autoru\.xml$ - https://yandex.ru/adv/products/display/maps-autoru
            permanent(h + '/media/banner/maps_autoru.xml', 'yandex.ru/adv/products/display/maps-autoru'),

            # ^/media/banner/market\.xml$ - https://yandex.ru/adv/products/display/marketmedia
            permanent(h + '/media/banner/market.xml', 'yandex.ru/adv/products/display/marketmedia'),

            # ^/media/banner/kinopoisk\.xml$ - https://yandex.ru/adv/products/display/kinopoiskmedia
            permanent(h + '/media/banner/kinopoisk.xml', 'yandex.ru/adv/products/display/kinopoiskmedia'),

            # ^/media/banner/video\.xml$ - https://yandex.ru/adv/requirements/videobanner
            permanent(h + '/media/banner/video.xml', 'yandex.ru/adv/requirements/videobanner'),

            # ^/media/banner/videoweb_yandex\.xml$ - https://yandex.ru/adv/products/display/videoweb
            permanent(h + '/media/banner/videoweb_yandex.xml', 'yandex.ru/adv/products/display/videoweb'),

            # ^/media/banner/crossmedia_network\.xml$ - https://yandex.ru/adv/products/display/cross-media-network
            permanent(h + '/media/banner/crossmedia_network.xml', 'yandex.ru/adv/products/display/cross-media-network'),

            # ^/media/context(/|/packet\.xml)?$ - https://yandex.ru/adv/products/context/contextdisplay
            # ^/media/context/?$ - https://yandex.ru/adv/products/context/contextdisplay
            permanent(h + '/media/context/', 'yandex.ru/adv/products/context/contextdisplay'),
            # ^/media/context/packet\.xml$ - https://yandex.ru/adv/products/context/contextdisplay
            permanent(h + '/media/context/packet.xml', 'yandex.ru/adv/products/context/contextdisplay'),

            # ^/media/settings/strategies\.xml$ - https://yandex.ru/adv/products/materials/strategies
            permanent(h + '/media/settings/strategies.xml', 'yandex.ru/adv/products/materials/strategies'),

            # ^/media/settings/(socdemserv|retarget_search|retarget|ltv|look-alike|interests|demography|targeting)\.xml$ - https://yandex.ru/adv/products/display
            # ^/media/settings/socdemserv\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/socdemserv.xml', 'yandex.ru/adv/products/display'),
            # ^/media/settings/retarget_search\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/retarget_search.xml', 'yandex.ru/adv/products/display'),
            # ^/media/settings/retarget\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/retarget.xml', 'yandex.ru/adv/products/display'),
            # ^/media/settings/ltv\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/ltv.xml', 'yandex.ru/adv/products/display'),
            # ^/media/settings/look-alike\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/look-alike.xml', 'yandex.ru/adv/products/display'),
            # ^/media/settings/interests\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/interests.xml', 'yandex.ru/adv/products/display'),
            # ^/media/settings/demography\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/demography.xml', 'yandex.ru/adv/products/display'),
            # ^/media/settings/targeting\.xml$ - https://yandex.ru/adv/products/display
            permanent(h + '/media/settings/targeting.xml', 'yandex.ru/adv/products/display'),

            # ^/media/(.*) - https://yandex.ru/adv/products/display
            permanent(h + '/media/*', 'yandex.ru/adv/products/display'),

            # ^/market/?$ - https://yandex.ru/adv/products/classified/market
            permanent(h + '/market/', 'yandex.ru/adv/products/classified/market'),

            # ^/products/displaycontext/(category|requirement)\.xml$ - https://yandex.ru/adv/products/context/contextdisplay
            # ^/products/displaycontext/category\.xml$ - https://yandex.ru/adv/products/context/contextdisplay
            permanent(h + '/products/displaycontext/category.xml', 'yandex.ru/adv/products/context/contextdisplay'),
            # ^/products/displaycontext/requirement\.xml$ - https://yandex.ru/adv/products/context/contextdisplay
            permanent(h + '/products/displaycontext/requirement.xml', 'yandex.ru/adv/products/context/contextdisplay'),

            # ^/discount\.xml$ - https://yandex.ru/adv/discount-context
            permanent(h + '/discount.xml', 'yandex.ru/adv/discount-context'),

            # ^/trebovaniya1\.xml$ - https://yandex.ru/legal/general_adv_rules/
            permanent(h + '/trebovaniya1.xml', 'yandex.ru/legal/general_adv_rules/'),

            # ^/trebovaniya2\.xml$ - https://yandex.ru/legal/banner_adv_rules/
            permanent(h + '/trebovaniya2.xml', 'yandex.ru/legal/banner_adv_rules/'),

            # ^/trebovaniya3\.xml$ - https://yandex.ru/adv/requirements/flash
            permanent(h + '/trebovaniya3.xml', 'yandex.ru/adv/requirements/flash'),

            # ^/politics\.xml$ - https://yandex.ru/adv/
            permanent(h + '/politics.xml', 'yandex.ru/adv/'),

            # ^/confidential/interests\.xml$ - https://tune.yandex.ru/adv/
            permanent(h + '/confidential/interests.xml', 'tune.yandex.ru/adv/'),

            # ^/confidential/users\.xml$ - https://yandex.ru/legal/confidential/
            permanent(h + '/confidential/users.xml', 'yandex.ru/legal/confidential/'),

            # ^/question\.xml$ - https://yandex.ru/adv/
            permanent(h + '/question.xml', 'yandex.ru/adv/'),

            # ^/event(\.xml)?/?(.*) - https://yandex.ru/adv/
            # ^/event\.xml/?(.*) - https://yandex.ru/adv/
            permanent(h + '/event.xml/*', 'yandex.ru/adv/'),
            # ^/event/?(.*) - https://yandex.ru/adv/
            permanent(h + '/event/*', 'yandex.ru/adv/'),

            # ^/(video|havas|gendir|disk-action|comdir) - https://yandex.ru/adv/
            # ^/video - https://yandex.ru/adv/
            permanent(h + '/video/*', 'yandex.ru/adv/'),
            # ^/havas - https://yandex.ru/adv/
            permanent(h + '/havas/*', 'yandex.ru/adv/'),
            # ^/gendir - https://yandex.ru/adv/
            permanent(h + '/gendir/*', 'yandex.ru/adv/'),
            # ^/disk-action - https://yandex.ru/adv/
            permanent(h + '/disk-action/*', 'yandex.ru/adv/'),
            # ^/comdir - https://yandex.ru/adv/
            permanent(h + '/comdir/*', 'yandex.ru/adv/'),
        ]

    for h in [
        str('adv.yandex.by'),
        str('advertising.yandex.by'),
        str('www.adv.yandex.by'),
        str('www.advertising.yandex.by'),
    ]:
        res += [
            # ^/news/(.*)/?$ - https://yandex.by/adv/news
            permanent(h + '/news/*', 'yandex.by/adv/news'),

            # ^/price/?$ - https://yandex.by/adv/
            permanent(h + '/price', 'yandex.by/adv/'),

            # ^/price\.xml$ - https://yandex.by/adv/prices
            permanent(h + '/price.xml', 'yandex.by/adv/prices'),

            # ^/price/context/direct\.xml$ - https://yandex.by/adv/products/context/search#price
            permanent(h + '/price/context/direct.xml', 'yandex.by/adv/products/context/search#price'),

            # ^/price/context/market\.xml$ - https://yandex.by/adv/products/classified/market
            permanent(h + '/price/context/market.xml', 'yandex.by/adv/products/classified/market'),

            # ^/price/context/sprav\.xml$ - https://yandex.by/adv/products/geo/spravprice
            permanent(h + '/price/context/sprav.xml', 'yandex.by/adv/products/geo/spravprice'),

            # ^/price/media/mainpage\.xml$ - https://yandex.by/adv/products/display/mainpage#price
            permanent(h + '/price/media/mainpage.xml', 'yandex.by/adv/products/display/mainpage#price'),

            # ^/price/media/(auction|auctionvideo|portal)\.xml$ - https://yandex.by/adv/products/display/auction#price
            # ^/price/media/auction\.xml$ - https://yandex.by/adv/products/display/auction#price
            permanent(h + '/price/media/auction.xml', 'yandex.by/adv/products/display/auction#price'),
            # ^/price/media/auctionvideo\.xml$ - https://yandex.by/adv/products/display/auction#price
            permanent(h + '/price/media/auctionvideo.xml', 'yandex.by/adv/products/display/auction#price'),
            # ^/price/media/portal\.xml$ - https://yandex.by/adv/products/display/auction#price
            permanent(h + '/price/media/portal.xml', 'yandex.by/adv/products/display/auction#price'),

            # ^/price/media/context\.xml$ - https://yandex.by/adv/products/context/contextdisplay#price
            permanent(h + '/price/media/context.xml', 'yandex.by/adv/products/context/contextdisplay#price'),

            # ^/price/media/kinopoisk\.xml$ - https://yandex.by/adv/products/display/kinopoiskmedia#price
            permanent(h + '/price/media/kinopoisk.xml', 'yandex.by/adv/products/display/kinopoiskmedia#price'),

            # ^/price/media/pda\.xml$ - https://yandex.by/adv/products/mobile/mobile-mainpage#price
            permanent(h + '/price/media/pda.xml', 'yandex.by/adv/products/mobile/mobile-mainpage#price'),

            # ^/price/discount/context\.xml$ - https://yandex.by/adv/discount-context
            permanent(h + '/price/discount/context.xml', 'yandex.by/adv/discount-context'),

            # ^/price/other/catalog\.xml$ - https://yandex.by/adv/products/classified/catalogue
            permanent(h + '/price/other/catalog.xml', 'yandex.by/adv/products/classified/catalogue'),

            # ^/price/other/courses\.xml$ - https://yandex.by/adv/courses
            permanent(h + '/price/other/courses.xml', 'yandex.by/adv/courses'),

            # ^/price/(.*) - https://yandex.by/adv/
            permanent(h + '/price/*', 'yandex.by/adv/'),

            # ^/requirement/media/videoweb\.xml$ - https://yandex.by/adv/requirements/videowebreq
            permanent(h + '/requirement/media/videoweb.xml', 'yandex.by/adv/requirements/videowebreq'),

            # ^/requirement/media/main\.xml$ - https://yandex.by/adv/requirements/mainpage
            permanent(h + '/requirement/media/main.xml', 'yandex.by/adv/requirements/mainpage'),

            # ^/requirement/media/flash\.xml$ - https://yandex.by/adv/requirements/flash
            permanent(h + '/requirement/media/flash.xml', 'yandex.by/adv/requirements/flash'),

            # ^/requirement/(.*) - https://yandex.by/adv/requirements
            permanent(h + '/requirement/*', 'yandex.by/adv/requirements'),

            # ^/context/direct/platforms\.xml$ - https://yandex.by/adv/products/materials/yan
            permanent(h + '/context/direct/platforms.xml', 'yandex.by/adv/products/materials/yan'),

            # ^/context/market/? - https://yandex.by/adv/products/classified/market
            permanent(h + '/context/market/', 'yandex.by/adv/products/classified/market'),

            # ^/context/? - https://yandex.by/adv/products/context
            permanent(h + '/context/', 'yandex.by/adv/products/context'),

            # ^/agency/collaboration/contract\.xml$ - https://yandex.by/adv/agency-contract
            permanent(h + '/agency/collaboration/contract.xml', 'yandex.by/adv/agency-contract'),

            # ^/agency/collaboration/request\.xml$ - https://yandex.by/adv/partners/request
            permanent(h + '/agency/collaboration/request.xml', 'yandex.by/adv/partners/request'),

            # ^/agency(/collaboration)?/?$ - https://yandex.by/adv/partners
            # ^/agency/collaboration/?$ - https://yandex.by/adv/partners
            permanent(h + '/agency/collaboration/', 'yandex.by/adv/partners'),
            # ^/agency/?$ - https://yandex.by/adv/partners
            permanent(h + '/agency/', 'yandex.by/adv/partners'),

            # ^/agency/status/(certification|requirement)\.xml$ - https://yandex.by/adv/partners/sertified-agency
            # ^/agency/status/certification\.xml$ - https://yandex.by/adv/partners/sertified-agency
            permanent(h + '/agency/status/certification.xml', 'yandex.by/adv/partners/sertified-agency'),
            # ^/agency/status/requirement\.xml$ - https://yandex.by/adv/partners/sertified-agency
            permanent(h + '/agency/status/requirement.xml', 'yandex.by/adv/partners/sertified-agency'),

            # ^/agency/status/seller\.xml$ - https://yandex.by/adv/order/mainpage/seller
            permanent(h + '/agency/status/seller.xml', 'yandex.by/adv/order/mainpage/seller'),

            # ^/agency/(.*) - https://yandex.by/adv/
            permanent(h + '/agency/*', 'yandex.by/adv/'),

            # ^/contact/agency/market/?$ - https://yandex.by/adv/contact/agency?cities[]=157&services[]=market
            permanent(h + '/contact/agency/market/', 'yandex.by/adv/contact/agency?cities[]=157&services[]=market'),

            # ^/contact/agency/partner\.xml$ - https://yandex.by/adv/contact/
            permanent(h + '/contact/agency/partner.xml', 'yandex.by/adv/contact/'),

            # ^/contact/agency/?(.*)/? - https://yandex.by/adv/contact/agency
            permanent(h + '/contact/agency/*', 'yandex.by/adv/contact/agency'),

            # ^/contact/yandex/order\.xml$ - https://yandex.by/adv/order/context/placead
            permanent(h + '/contact/yandex/order.xml', 'yandex.by/adv/order/context/placead'),

            # ^/contact/yandex/sale\.xml$ - https://yandex.by/adv/contact/offices
            permanent(h + '/contact/yandex/sale.xml', 'yandex.by/adv/contact/offices'),

            # ^/contact/?$ - https://yandex.by/adv/contact
            permanent(h + '/contact/', 'yandex.by/adv/contact'),

            # ^/contact/(.*) - https://yandex.by/adv/
            permanent(h + '/contact/*', 'yandex.by/adv/'),

            # ^/advertiser/(.*)/?$ - https://yandex.by/adv/edu
            permanent(h + '/advertiser/*', 'yandex.by/adv/edu'),

            # ^/media/banner/pda\.xml$ - https://yandex.by/adv/products/mobile/mobile-mainpage
            permanent(h + '/media/banner/pda.xml', 'yandex.by/adv/products/mobile/mobile-mainpage'),

            # ^/media/banner/mainpage\.xml$ - https://yandex.by/adv/products/display/mainpage
            permanent(h + '/media/banner/mainpage.xml', 'yandex.by/adv/products/display/mainpage'),

            # ^/media/banner/auction\.xml$ - https://yandex.by/adv/products/display/auction
            permanent(h + '/media/banner/auction.xml', 'yandex.by/adv/products/display/auction'),

            # ^/media/banner/kinopoisk\.xml$ - https://yandex.by/adv/products/display/kinopoiskmedia
            permanent(h + '/media/banner/kinopoisk.xml', 'yandex.by/adv/products/display/kinopoiskmedia'),

            # ^/media/banner/videoweb_yandex\.xml$ - https://yandex.by/adv/requirements/videoweb
            permanent(h + '/media/banner/videoweb_yandex.xml', 'yandex.by/adv/requirements/videoweb'),

            # ^/media/context/?$ - https://yandex.by/adv/products/context/contextdisplay
            permanent(h + '/media/context/', 'yandex.by/adv/products/context/contextdisplay'),

            # ^/media/context/packet\.xml$ - https://yandex.by/adv/
            permanent(h + '/media/context/packet.xml', 'yandex.by/adv/'),

            # ^/media/(.*)/? - https://yandex.by/adv/products/display
            permanent(h + '/media/*', 'yandex.by/adv/products/display'),

            # ^/context/direct/requirement\.xml$ - https://yandex.by/legal/direct_adv_rules/
            permanent(h + '/context/direct/requirement.xml', 'yandex.by/legal/direct_adv_rules/'),

            # ^/requirement/media/banner\.xml$ - https://yandex.by/legal/banner_adv_rules/
            permanent(h + '/requirement/media/banner.xml', 'yandex.by/legal/banner_adv_rules/'),

            # ^/context/direct/notice\.xml$ - https://yandex.by/support/direct/troubleshooting/advices.xml
            permanent(h + '/context/direct/notice.xml', 'yandex.by/support/direct/troubleshooting/advices.xml'),

            # ^/context/direct/help\.xml$ - https://yandex.by/support/direct/order-ru.xml
            permanent(h + '/context/direct/help.xml', 'yandex.by/support/direct/order-ru.xml'),

            # ^/context/direct/order\.xml$ - https://yandex.by/support/direct/order-ru.xml
            permanent(h + '/context/direct/order.xml', 'yandex.by/support/direct/order-ru.xml'),

            # ^/contact/yandex/support\.xml$ - https://yandex.by/adv/contact
            permanent(h + '/contact/yandex/support.xml', 'yandex.by/adv/contact'),
        ]

    for h in [
        str('adv.yandex.kz'),
        str('advertising.yandex.kz'),
        str('www.adv.yandex.kz'),
        str('www.advertising.yandex.kz'),
    ]:
        res += [
            # ^/news/(.*)/?$ - https://yandex.kz/adv/news
            permanent(h + '/news/*', 'yandex.kz/adv/news'),

            # ^/price/?$ - https://yandex.kz/adv/prices-common
            permanent(h + '/price/', 'yandex.kz/adv/prices-common'),

            # ^/price\.xml$ - https://yandex.kz/adv/prices
            permanent(h + '/price.xml', 'yandex.kz/adv/prices'),

            # ^/price/context/direct\.xml$ - https://yandex.kz/adv/products/context/search#price
            permanent(h + '/price/context/direct.xml', 'yandex.kz/adv/products/context/search#price'),

            # ^/price/context/market\.xml$ - https://yandex.kz/adv/products/classified/market
            permanent(h + '/price/context/market.xml', 'yandex.kz/adv/products/classified/market'),

            # ^/price/context/sprav\.xml$ - https://yandex.kz/adv/products/geo/spravprice
            permanent(h + '/price/context/sprav.xml', 'yandex.kz/adv/products/geo/spravprice'),

            # ^/price/media/mainpage\.xml$ - https://yandex.kz/adv/products/display/mainpage#price
            permanent(h + '/price/media/mainpage.xml', 'yandex.kz/adv/products/display/mainpage#price'),

            # ^/price/media/(weather|maps|lifestyle|auction|)\.xml$ - https://yandex.kz/adv/products/display/$1#price
            # ^/price/media/weather\.xml$ - https://yandex.kz/adv/products/display/weather#price
            permanent(h + '/price/media/weather.xml', 'yandex.kz/adv/products/display/weather#price'),
            # ^/price/media/maps\.xml$ - https://yandex.kz/adv/products/display/maps#price
            permanent(h + '/price/media/maps.xml', 'yandex.kz/adv/products/display/maps#price'),
            # ^/price/media/lifestyle\.xml$ - https://yandex.kz/adv/products/display/lifestyle#price
            permanent(h + '/price/media/lifestyle.xml', 'yandex.kz/adv/products/display/lifestyle#price'),
            # ^/price/media/auction\.xml$ - https://yandex.kz/adv/products/display/auction#price
            permanent(h + '/price/media/auction.xml', 'yandex.kz/adv/products/display/auction#price'),
            # ^/price/media/\.xml$ - https://yandex.kz/adv/products/display/#price
            permanent(h + '/price/media/.xml', 'yandex.kz/adv/products/display/#price'),

            # ^/price/media/context\.xml$ - https://yandex.kz/adv/products/context/contextdisplay#price
            permanent(h + '/price/media/context.xml', 'yandex.kz/adv/products/context/contextdisplay#price'),

            # ^/price/media/kinopoisk\.xml$ - https://yandex.kz/adv/products/display/kinopoiskmedia#price
            permanent(h + '/price/media/kinopoisk.xml', 'yandex.kz/adv/products/display/kinopoiskmedia#price'),

            # ^/price/discount/context\.xml$ - https://yandex.kz/adv/discount-context
            permanent(h + '/price/discount/context.xml', 'yandex.kz/adv/discount-context'),

            # ^/price/discount/media\.xml$ - https://yandex.kz/adv/prices-common
            permanent(h + '/price/discount/media.xml', 'yandex.kz/adv/prices-common'),

            # ^/price/media/(mail|portal|schedule)\.xml$ - https://yandex.kz/adv/prices-common
            # ^/price/media/mail\.xml$ - https://yandex.kz/adv/prices-common
            permanent(h + '/price/media/mail.xml', 'yandex.kz/adv/prices-common'),
            # ^/price/media/portal\.xml$ - https://yandex.kz/adv/prices-common
            permanent(h + '/price/media/portal.xml', 'yandex.kz/adv/prices-common'),
            # ^/price/media/schedule\.xml$ - https://yandex.kz/adv/prices-common
            permanent(h + '/price/media/schedule.xml', 'yandex.kz/adv/prices-common'),

            # ^/requirement/media/flash\.xml$ - https://yandex.kz/adv/requirements/flash
            permanent(h + '/requirement/media/flash.xml', 'yandex.kz/adv/requirements/flash'),

            # ^/requirement/media/html5\.xml$ - https://yandex.kz/adv/requirements/html5
            permanent(h + '/requirement/media/html5.xml', 'yandex.kz/adv/requirements/html5'),

            # ^/requirement/media/main\.xml$ - https://yandex.kz/adv/requirements/mainpage
            permanent(h + '/requirement/media/main.xml', 'yandex.kz/adv/requirements/mainpage'),

            # ^/context/direct/platforms\.xml$ - https://yandex.kz/adv/products/materials/yan
            permanent(h + '/context/direct/platforms.xml', 'yandex.kz/adv/products/materials/yan'),

            # ^/context/sprav/? - https://yandex.kz/adv/products/geo
            permanent(h + '/context/sprav/', 'yandex.kz/adv/products/geo'),

            # ^/context/market/? - https://yandex.kz/adv/products/classified/market
            permanent(h + '/context/market/', 'yandex.kz/adv/products/classified/market'),

            # ^/context/? - https://yandex.kz/adv/products/context
            permanent(h + '/context/', 'yandex.kz/adv/products/context'),

            # ^/agency/collaboration/request\.xml$ - https://yandex.kz/adv/partners/request
            permanent(h + '/agency/collaboration/request.xml', 'yandex.kz/adv/partners/request'),

            # ^/agency(/collaboration)?/?$ - https://yandex.kz/adv/partners
            # ^/agency/collaboration/?$ - https://yandex.kz/adv/partners
            permanent(h + '/agency/collaboration/', 'yandex.kz/adv/partners'),
            # ^/agency/?$ - https://yandex.kz/adv/partners
            permanent(h + '/agency/', 'yandex.kz/adv/partners'),

            # ^/agency/status/(certification|requirement)\.xml$ - https://yandex.kz/adv/partners
            # ^/agency/status/certification\.xml$ - https://yandex.kz/adv/partners
            permanent(h + '/agency/status/certification.xml', 'yandex.kz/adv/partners'),
            # ^/agency/status/requirement\.xml$ - https://yandex.kz/adv/partners
            permanent(h + '/agency/status/requirement.xml', 'yandex.kz/adv/partners'),

            # ^/agency/(.*) - https://yandex.kz/adv/
            permanent(h + '/agency/*', 'yandex.kz/adv/'),

            # ^/contact/yandex/order\.xml$ - https://yandex.kz/adv/order/context/placead
            permanent(h + '/contact/yandex/order.xml', 'yandex.kz/adv/order/context/placead'),

            # ^/contact/?$ - https://yandex.kz/adv/contact
            permanent(h + '/contact/', 'yandex.kz/adv/contact'),

            # ^/contact/agency/partner\.xml - https://yandex.kz/adv/
            permanent(h + '/contact/agency/partner.xml', 'yandex.kz/adv/'),

            # ^/contact/agency/?(.*)/? - https://yandex.kz/adv/contact/agency?cities[]=162&cities[]=10295
            permanent(h + '/contact/agency/*', 'yandex.kz/adv/contact/agency?cities[]=162&cities[]=10295'),

            # ^/advertiser/(.*)/?$ - https://yandex.kz/adv/edu
            permanent(h + '/advertiser/*', 'yandex.kz/adv/edu'),

            # ^/media/banner/mainpage\.xml$ - https://yandex.kz/adv/products/display/mainpage
            permanent(h + '/media/banner/mainpage.xml', 'yandex.kz/adv/products/display/mainpage'),

            # ^/media/banner/weather\.xml$ - https://yandex.kz/adv/products/display/weather
            permanent(h + '/media/banner/weather.xml', 'yandex.kz/adv/products/display/weather'),

            # ^/media/banner/maps\.xml$ - https://yandex.kz/adv/products/display/maps
            permanent(h + '/media/banner/maps.xml', 'yandex.kz/adv/products/display/maps'),

            # ^/media/banner/lifestyle\.xml$ - https://yandex.kz/adv/products/display/lifestyle
            permanent(h + '/media/banner/lifestyle.xml', 'yandex.kz/adv/products/display/lifestyle'),

            # ^/media/banner/auction\.xml$ - https://yandex.kz/adv/products/display/auction
            permanent(h + '/media/banner/auction.xml', 'yandex.kz/adv/products/display/auction'),

            # ^/media/banner/kinopoisk\.xml$ - https://yandex.kz/adv/products/display/kinopoiskmedia
            permanent(h + '/media/banner/kinopoisk.xml', 'yandex.kz/adv/products/display/kinopoiskmedia'),

            # ^/media/context/?$ - https://yandex.kz/adv/products/context/contextdisplay
            permanent(h + '/media/context/', 'yandex.kz/adv/products/context/contextdisplay'),

            # ^/media/(.*)/? - https://yandex.kz/adv/products/display
            permanent(h + '/media/*', 'yandex.kz/adv/products/display'),

            # ^/context/direct/requirement\.xml$ - https://yandex.kz/legal/direct_adv_rules/
            permanent(h + '/context/direct/requirement.xml', 'yandex.kz/legal/direct_adv_rules/'),

            # ^/media/context/requirement\.xml$ - https://yandex.kz/legal/requirements_media_context_banner/
            permanent(h + '/media/context/requirement.xml', 'yandex.kz/legal/requirements_media_context_banner/'),

            # ^/requirement/context/general\.xml$ - https://yandex.kz/legal/general_adv_rules/
            permanent(h + '/requirement/context/general.xml', 'yandex.kz/legal/general_adv_rules/'),

            # ^/requirement/context/text\.xml$ - https://yandex.kz/legal/text_adv_rules/
            permanent(h + 'requirement/context/text.xml', 'yandex.kz/legal/text_adv_rules/'),

            # ^/requirement/context/regulations\.xml$ - https://yandex.kz/legal/adv_rules/
            permanent(h + '/requirement/context/regulations.xml', 'yandex.kz/legal/adv_rules/'),

            # ^/requirement/media/general\.xml$ - https://yandex.kz/legal/general_adv_rules/
            permanent(h + '/requirement/media/general.xml', 'yandex.kz/legal/general_adv_rules/'),

            # ^/requirement/media/banner\.xml$ - https://yandex.kz/legal/banner_adv_rules/
            permanent(h + '/requirement/media/banner.xml', 'yandex.kz/legal/banner_adv_rules/'),

            # ^/requirement/media/regulations\.xml$ - https://yandex.kz/legal/adv_rules/
            permanent(h + '/requirement/media/regulations.xml', 'yandex.kz/legal/adv_rules/'),

            # ^/media/banner/news\.xml$ - https://yandex.kz/adv/products/display
            permanent(h + '/media/banner/news.xml', 'yandex.kz/adv/products/display'),

            # ^/price/media/news\.xml$ - https://yandex.kz/adv/prices-common
            permanent(h + '/price/media/news.xml', 'yandex.kz/adv/prices-common'),

            # ^/contact/yandex/(sale|support)\.xml$ - https://yandex.kz/adv/contact
            # ^/contact/yandex/sale\.xml$ - https://yandex.kz/adv/contact
            permanent(h + '/contact/yandex/sale.xml', 'yandex.kz/adv/contact'),
            # ^/contact/yandex/support\.xml$ - https://yandex.kz/adv/contact
            permanent(h + 'contact/yandex/support.xml', 'yandex.kz/adv/contact'),

            # ^/context/direct/notice\.xml$ - https://yandex.kz/support/direct/troubleshooting/advices.xml
            permanent(h + 'context/direct/notice.xml', 'yandex.kz/support/direct/troubleshooting/advices.xml'),

            # ^/context/direct/help\.xml$ - https://yandex.kz/support/direct/order-ru.xml
            permanent(h + '/context/direct/help.xml', 'yandex.kz/support/direct/order-ru.xml'),

            # ^/context/direct/order\.xml$ - https://yandex.kz/support/direct/order-ru.xml
            permanent(h + '/context/direct/order.xml', 'yandex.kz/support/direct/order-ru.xml'),
        ]

    for h in [
        str('adv.yandex.ua'),
        str('advertising.yandex.ua'),
        str('www.adv.yandex.ua'),
        str('www.advertising.yandex.ua'),
    ]:
        res += [
            # ^/news/(.*)/?$ - https://yandex.ua/adv/news
            # ^/news(.*)\.xml$ - https://yandex.ua/adv/news
            permanent(h + '/news/*', 'yandex.ua/adv/news'),

            # ^/advertiser/(.*) - https://yandex.ua/adv/edu/
            permanent(h + '/advertiser/*', 'yandex.ua/adv/edu/'),

            # ^/agency/(.*) - https://yandex.ua/adv/partners/
            permanent(h + '/agency/*', 'yandex.ua/adv/partners/'),

            # ^/price/?$ - https://yandex.ua/adv/prices-common
            permanent(h + '/price/', 'yandex.ua/adv/prices-common'),

            # ^/price\.xml$ - https://yandex.ua/adv/prices
            permanent(h + '/price.xml', 'yandex.ua/adv/prices'),

            # ^/price/context/catalog\.xml$ - https://yandex.ua/adv/products/classified/catalogue#price
            permanent(h + '/price/context/catalog.xml', 'yandex.ua/adv/products/classified/catalogue#price'),

            # ^/price/context/direct\.xml$ - https://yandex.ua/adv/products/context/search#price
            permanent(h + '/price/context/direct.xml', 'yandex.ua/adv/products/context/search#price'),

            # ^/price/context/market\.xml$ - https://yandex.ua/adv/products/classified/market
            permanent(h + '/price/context/market.xml', 'yandex.ua/adv/products/classified/market'),

            # ^/price/context/sprav\.xml$ - https://yandex.ua/adv/products/geo/spravprice
            permanent(h + '/price/context/sprav.xml', 'yandex.ua/adv/products/geo/spravprice'),

            # ^/price/media/mainpage\.xml$ - https://yandex.ua/adv/products/display/mainpage#price
            permanent(h + '/price/media/mainpage.xml', 'yandex.ua/adv/products/display/mainpage#price'),

            # ^/price/media/(auction|auctionvideo)\.xml$ - https://yandex.ua/adv/products/display/auction#price
            # ^/price/media/auction\.xml$ - https://yandex.ua/adv/products/display/auction#price
            permanent(h + '/price/media/auction.xml', 'yandex.ua/adv/products/display/auction#price'),
            # ^/price/media/auctionvideo\.xml$ - https://yandex.ua/adv/products/display/auction#price
            permanent(h + '/price/media/auctionvideo.xml', 'yandex.ua/adv/products/display/auction#price'),

            # ^/price/media/context\.xml$ - https://yandex.ua/adv/products/context/contextdisplay#price
            permanent(h + '/price/media/context.xml', 'yandex.ua/adv/products/context/contextdisplay#price'),

            # ^/price/media/kinopoisk\.xml$ - https://yandex.ua/adv/products/display/kinopoiskmedia#price
            permanent(h + '/price/media/kinopoisk.xml', 'yandex.ua/adv/products/display/kinopoiskmedia#price'),

            # ^/price/media/pda\.xml$ - https://yandex.ua/adv/products/mobile/mobile-mainpage#price
            permanent(h + '/price/media/pda.xml', 'yandex.ua/adv/products/mobile/mobile-mainpage#price'),

            # ^/price/discount/context\.xml$ - https://yandex.ua/adv/products/geo/spravprice
            permanent(h + '/price/discount/context.xml', 'yandex.ua/adv/products/geo/spravprice'),

            # ^/price/other/catalog\.xml$ - https://yandex.ua/adv/products/classified/catalogue
            permanent(h + '/price/other/catalog.xml', 'yandex.ua/adv/products/classified/catalogue'),

            # ^/price/other/courses\.xml$ - https://yandex.ua/adv/courses
            permanent(h + '/price/other/courses.xml', 'yandex.ua/adv/courses'),

            # ^/price/(.*) - https://yandex.ua/adv/prices-common
            permanent(h + '/price/*', 'yandex.ua/adv/prices-common'),

            # ^/requirement/media/flash\.xml$ - https://yandex.ua/adv/requirements/flash
            permanent(h + '/requirement/media/flash.xml', 'yandex.ua/adv/requirements/flash'),

            # ^/requirement/media/html5\.xml$ - https://yandex.ua/adv/requirements/html5
            permanent(h + '/requirement/media/html5.xml', 'yandex.ua/adv/requirements/html5'),

            # ^/requirement/media/main\.xml$ - https://yandex.ua/adv/requirements/mainpage
            permanent(h + '/requirement/media/main.xml', 'yandex.ua/adv/requirements/mainpage'),

            # ^/context/direct/platforms\.xml$ - https://yandex.ua/adv/products/materials/yan
            permanent(h + '/context/direct/platforms.xml', 'yandex.ua/adv/products/materials/yan'),

            # ^/context/sprav/? - https://yandex.ua/adv/products/geo
            permanent(h + '/context/sprav/', 'yandex.ua/adv/products/geo'),

            # ^/context/market/? - https://yandex.ua/adv/products/classified/market
            permanent(h + '/context/market/', 'yandex.ua/adv/products/classified/market'),

            # ^/context/? - https://yandex.ua/adv/products/context
            permanent(h + '/context/', 'yandex.ua/adv/products/context'),

            # ^/agency/collaboration/contract\.xml$ - https://yandex.ua/adv/agency-contract
            permanent(h + '/agency/collaboration/contract.xml', 'yandex.ua/adv/agency-contract'),

            # ^/agency/collaboration/request\.xml$ - https://yandex.ua/adv/partners/request
            permanent(h + '/agency/collaboration/request.xml', 'yandex.ua/adv/partners/request'),

            # ^/agency(/collaboration)?/?$ - https://yandex.ua/adv/partners
            # ^/agency/collaboration/?$ - https://yandex.ua/adv/partners
            permanent(h + '/agency/collaboration/', 'yandex.ua/adv/partners'),
            # ^/agency/?$ - https://yandex.ua/adv/partners
            permanent(h + '/agency/', 'yandex.ua/adv/partners'),

            # ^/agency/status/(certification|requirement)\.xml$ - https://yandex.ua/adv/partners/sertified-agency
            # ^/agency/status/certification\.xml$ - https://yandex.ua/adv/partners/sertified-agency
            permanent(h + '/agency/status/certification.xml', 'yandex.ua/adv/partners/sertified-agency'),
            # ^/agency/status/requirement\.xml$ - https://yandex.ua/adv/partners/sertified-agency
            permanent(h + '/agency/status/requirement.xml', 'yandex.ua/adv/partners/sertified-agency'),

            # ^/contact/?$ - https://yandex.ua/adv/contact
            permanent(h + '/contact/', 'yandex.ua/adv/contact'),

            # ^/contact/yandex/sale\.xml$ - https://yandex.ua/adv/contact/offices?country=ukraine
            permanent(h + '/contact/yandex/sale.xml', 'yandex.ua/adv/contact/offices?country=ukraine'),

            # ^/contact/yandex/order\.xml$ - https://yandex.ua/adv/order/context/placead
            permanent(h + '/contact/yandex/order.xml', 'yandex.ua/adv/order/context/placead'),

            # ^/contact/agency/partner\.xml$ - https://yandex.ua/adv/partners/
            permanent(h + '/contact/agency/partner.xml', 'yandex.ua/adv/partners/'),

            # ^/contact/yandex/support\.xml$ - https://yandex.ua/adv/contact/
            permanent(h + '/contact/yandex/support.xml', 'yandex.ua/adv/contact/'),

            # ^/market/ - https://yandex.ua/adv/products/classified/market
            permanent(h + '/market/', 'yandex.ua/adv/products/classified/market'),

            # ^/media/banner/pda\.xml$ - https://yandex.ua/adv/products/mobile/mobile-mainpage
            permanent(h + '/media/banner/pda.xml', 'yandex.ua/adv/products/mobile/mobile-mainpage'),

            # ^/media/banner/mainpage\.xml$ - https://yandex.ua/adv/products/display/mainpage
            permanent(h + '/media/banner/mainpage.xml', 'yandex.ua/adv/products/display/mainpage'),

            # ^/media/banner/auction\.xml$ - https://yandex.ua/adv/products/display/auction
            permanent(h + '/media/banner/auction.xml', 'yandex.ua/adv/products/display/auction'),

            # ^/media/banner/kinopoisk\.xml$ - https://yandex.ua/adv/products/display/kinopoiskmedia
            permanent(h + '/media/banner/kinopoisk.xml', 'yandex.ua/adv/products/display/kinopoiskmedia'),

            # ^/media/banner/video\.xml$ - https://yandex.ua/adv/requirements/videobanner
            permanent(h + '/media/banner/video.xml', 'yandex.ua/adv/requirements/videobanner'),

            # ^/media/context/?$ - https://yandex.ua/adv/products/context/contextdisplay
            permanent(h + '/media/context/', 'yandex.ua/adv/products/context/contextdisplay'),

            # ^/media/(.*)/? - https://yandex.ua/adv/products/display
            permanent(h + '/media/*', 'yandex.ua/adv/products/display'),

            # ^/(afisha|adv404|advauto|image|advcards|advnews|maps|mail|market2|slovari|traffic|weather)\.xml$ - https://yandex.ua/adv/products/display
            # ^/afisha\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/afisha.xml', 'yandex.ua/adv/products/display'),
            # ^/adv404\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/adv404.xml', 'yandex.ua/adv/products/display'),
            # ^/advauto\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/advauto.xml', 'yandex.ua/adv/products/display'),
            # ^/image\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/image.xml', 'yandex.ua/adv/products/display'),
            # ^/advcards\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/advcards.xml', 'yandex.ua/adv/products/display'),
            # ^/advnews\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/advnews.xml', 'yandex.ua/adv/products/display'),
            # ^/maps\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/maps.xml', 'yandex.ua/adv/products/display'),
            # ^/mail\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/mail.xml', 'yandex.ua/adv/products/display'),
            # ^/market2\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/market2.xml', 'yandex.ua/adv/products/display'),
            # ^/slovari\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/slovari.xml', 'yandex.ua/adv/products/display'),
            # ^/traffic\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/traffic.xml', 'yandex.ua/adv/products/display'),
            # ^/weather\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/weather.xml', 'yandex.ua/adv/products/display'),

            # ^/media/banner/cards\.xml$ - https://yandex.ua/adv/products/display
            permanent(h + '/media/banner/cards.xml', 'yandex.ua/adv/products/display'),

            # ^/homepage\.xml$ - https://yandex.ua/adv/products/display/mainpage
            permanent(h + '/homepage.xml', 'yandex.ua/adv/products/display/mainpage'),

            # ^/(medcontextreq|parcel)\.xml$ - https://yandex.ua/adv/products/context/contextdisplay
            # ^/medcontextreq\.xml$ - https://yandex.ua/adv/products/context/contextdisplay
            permanent(h + 'medcontextreq.xml', 'yandex.ua/adv/products/context/contextdisplay'),
            # ^/parcel\.xml$ - https://yandex.ua/adv/products/context/contextdisplay
            permanent(h + '/parcel.xml', 'yandex.ua/adv/products/context/contextdisplay'),

            # ^/(kupislova|spetsrazm|sea|seafaq|seafaqmore|)\.xml$ - https://yandex.ua/adv/products/context
            # ^/kupislova\.xml$ - https://yandex.ua/adv/products/context
            permanent(h + '/kupislova.xml', 'yandex.ua/adv/products/context'),
            # ^/spetsrazm\.xml$ - https://yandex.ua/adv/products/context
            permanent(h + '/spetsrazm.xml', 'yandex.ua/adv/products/context'),
            # ^/sea\.xml$ - https://yandex.ua/adv/products/context
            permanent(h + '/sea.xml', 'yandex.ua/adv/products/context'),
            # ^/seafaq\.xml$ - https://yandex.ua/adv/products/context
            permanent(h + '/seafaq.xml', 'yandex.ua/adv/products/context'),
            # ^/seafaqmore\.xml$ - https://yandex.ua/adv/products/context
            permanent(h + '/seafaqmore.xml', 'yandex.ua/adv/products/context'),
            # ^/\.xml$ - https://yandex.ua/adv/products/context
            permanent(h + '/.xml', 'yandex.ua/adv/products/context'),

            # ^/orderform\.xml$ - https://yandex.ua/adv/order/context/placead
            permanent(h + '/orderform.xml', 'yandex.ua/adv/order/context/placead'),

            # ^/pricecat\.xml$ - https://yandex.ua/adv/products/classified/catalogue
            permanent(h + '/pricecat.xml', 'yandex.ua/adv/products/classified/catalogue'),

            # ^/trebovaniya(\d?)\.xml$ - https://yandex.ua/adv/requirements
            # ^/trebovaniya\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya0\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya0.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya1\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya1.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya2\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya2.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya3\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya3.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya4\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya4.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya5\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya5.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya6\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya6.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya7\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya7.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya8\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya8.xml', 'yandex.ua/adv/requirements'),
            # ^/trebovaniya9\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/trebovaniya9.xml', 'yandex.ua/adv/requirements'),

            # ^/media/context/requirement\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/media/context/requirement.xml', 'yandex.ua/adv/requirements'),

            # ^/context/direct/requirement\.xml$ - https://yandex.ua/adv/requirements
            permanent(h + '/context/direct/requirement.xml', 'yandex.ua/adv/requirements'),

            # ^/requirement/(.*)$ - https://yandex.ua/adv/requirements
            permanent(h + '/requirement/*', 'yandex.ua/adv/requirements'),
        ]

    for h in [
        str('adv.yandex.com'),
        str('advertising.yandex.com'),
        str('www.adv.yandex.com'),
        str('www.advertising.yandex.com'),
    ]:
        res += [
            # ^/news/(.*) - https://yandex.com/adv/news
            permanent(h + '/news/*', 'yandex.com/adv/news'),

            # ^/product/context/direct\.xml$ - https://yandex.com/adv/products/context/search
            permanent(h + '/product/context/direct.xml', 'yandex.com/adv/products/context/search'),

            # ^/product/context/market\.xml$ - https://yandex.com/adv/products/classified/market
            permanent(h + '/product/context/market.xml', 'yandex.com/adv/products/classified/market'),

            # ^/product/media/homepage\.xml$ - https://yandex.com/adv/products/display/mainpage
            permanent(h + '/product/media/homepage.xml', 'yandex.com/adv/products/display/mainpage'),

            # ^/product/media/(weather|maps|avia|realty|travel|afisha|music|afishamusic|lifestyle)\.xml$ - https://yandex.com/adv/products/display/$1
            # ^/product/media/weather\.xml$ - https://yandex.com/adv/products/display/weather
            permanent(h + '/product/media/weather.xml', 'yandex.com/adv/products/display/weather'),
            # ^/product/media/maps\.xml$ - https://yandex.com/adv/products/display/maps
            permanent(h + '/product/media/maps.xml', 'yandex.com/adv/products/display/maps'),
            # ^/product/media/avia\.xml$ - https://yandex.com/adv/products/display/avia
            permanent(h + '/product/media/avia.xml', 'yandex.com/adv/products/display/avia'),
            # ^/product/media/realty\.xml$ - https://yandex.com/adv/products/display/realty
            permanent(h + '/product/media/realty.xml', 'yandex.com/adv/products/display/realty'),
            # ^/product/media/travel\.xml$ - https://yandex.com/adv/products/display/travel
            permanent(h + '/product/media/travel.xml', 'yandex.com/adv/products/display/travel'),
            # ^/product/media/afisha\.xml$ - https://yandex.com/adv/products/display/afisha
            permanent(h + '/product/media/afisha.xml', 'yandex.com/adv/products/display/afisha'),
            # ^/product/media/music\.xml$ - https://yandex.com/adv/products/display/music
            permanent(h + '/product/media/music.xml', 'yandex.com/adv/products/display/music'),
            # ^/product/media/afishamusic\.xml$ - https://yandex.com/adv/products/display/afishamusic
            permanent(h + '/product/media/afishamusic.xml', 'yandex.com/adv/products/display/afishamusic'),
            # ^/product/media/lifestyle\.xml$ - https://yandex.com/adv/products/display/lifestyle
            permanent(h + '/product/media/lifestyle.xml', 'yandex.com/adv/products/display/lifestyle'),

            # ^/product/media/(market|marketbanner)\.xml$ - https://yandex.com/adv/products/display/marketmedia
            # ^/product/media/market\.xml$ - https://yandex.com/adv/products/display/marketmedia
            permanent(h + '/product/media/market.xml', 'yandex.com/adv/products/display/marketmedia'),
            # ^/product/media/marketbanner\.xml$ - https://yandex.com/adv/products/display/marketmedia
            permanent(h + '/product/media/marketbanner.xml', 'yandex.com/adv/products/display/marketmedia'),

            # ^/product/media/timetables\.xml$ - https://yandex.com/adv/products/display/schedule
            permanent(h + '/product/media/timetables.xml', 'yandex.com/adv/products/display/schedule'),

            # ^/product/media/jobs\.xml$ - https://yandex.com/adv/products/display/rabota
            permanent(h + '/product/media/jobs.xml', 'yandex.com/adv/products/display/rabota'),

            # ^/product/media/kinopoisk\.xml$ - https://yandex.com/adv/products/display/kinopoiskmedia
            permanent(h + '/product/media/kinopoisk.xml', 'yandex.com/adv/products/display/kinopoiskmedia'),

            # ^/product/media/videoweb_yandex\.xml$ - https://yandex.com/adv/products/display/videoweb
            permanent(h + '/product/media/videoweb_yandex.xml', 'yandex.com/adv/products/display/videoweb'),

            # ^/product/audio/radio\.xml$ - https://yandex.com/adv/products/display/audio
            permanent(h + '/product/audio/radio.xml', 'yandex.com/adv/products/display/audio'),

            # ^/product/media/crossmedia_network\.xml$ - https://yandex.com/adv/products/display/cross-media-network
            permanent(h + '/product/media/crossmedia_network.xml', 'yandex.com/adv/products/display/cross-media-network'),

            # ^/product/media/contextual\.xml$ - https://yandex.com/adv/products/context/contextdisplay
            permanent(h + '/product/media/contextual.xml', 'yandex.com/adv/products/context/contextdisplay'),

            # ^/product/media/(auction|interests|retarget|network)\.xml$ - https://yandex.com/adv/products/display/auction
            # ^/product/media/auction\.xml$ - https://yandex.com/adv/products/display/auction
            permanent(h + '/product/media/auction.xml', 'yandex.com/adv/products/display/auction'),
            # ^/product/media/interests\.xml$ - https://yandex.com/adv/products/display/auction
            permanent(h + '/product/media/interests.xml', 'yandex.com/adv/products/display/auction'),
            # ^/product/media/retarget\.xml$ - https://yandex.com/adv/products/display/auction
            permanent(h + '/product/media/retarget.xml', 'yandex.com/adv/products/display/auction'),
            # ^/product/media/network\.xml$ - https://yandex.com/adv/products/display/auction
            permanent(h + '/product/media/network.xml', 'yandex.com/adv/products/display/auction'),

            # ^/product/media/video\.xml$ - https://yandex.com/adv/requirements/videobanner
            permanent(h + '/product/media/video.xml', 'yandex.com/adv/requirements/videobanner'),

            # ^/product/media/look-alike\.xml$ - https://yandex.com/adv/products/display/audience
            permanent(h + '/product/media/look-alike.xml', 'yandex.com/adv/products/display/audience'),

            # ^/product/media/mobile_homepage\.xml$ - https://yandex.com/adv/products/mobile/mobile-mainpage
            permanent(h + '/product/media/mobile_homepage.xml', 'yandex.com/adv/products/mobile/mobile-mainpage'),

            # ^/product/media/(auction_mobilegeo|mobile)\.xml$ - https://yandex.com/adv/products/mobile/mobileauction
            # ^/product/media/auction_mobilegeo\.xml$ - https://yandex.com/adv/products/mobile/mobileauction
            permanent(h + '/product/media/auction_mobilegeo.xml', 'yandex.com/adv/products/mobile/mobileauction'),
            # ^/product/media/mobile\.xml$ - https://yandex.com/adv/products/mobile/mobileauction
            permanent(h + '/product/media/mobile.xml', 'yandex.com/adv/products/mobile/mobileauction'),

            # ^/product/price/?$ - https://yandex.com/adv/prices
            permanent(h + '/product/price/', 'yandex.com/adv/prices'),

            # ^/price/context/direct\.xml$ - https://yandex.com/adv/products/context/search#price
            permanent(h + '/price/context/direct.xml', 'yandex.com/adv/products/context/search#price'),

            # ^/price/context/market\.xml$ - https://yandex.com/adv/products/classified/market#price
            permanent(h + '/price/context/market.xml', 'yandex.com/adv/products/classified/market#price'),

            # ^/price/other/catalog\.xml$ - https://yandex.com/adv/products/classified/catalogue
            permanent(h + '/price/other/catalog.xml', 'yandex.com/adv/products/classified/catalogue'),

            # ^/price/media/homepage\.xml$ - https://yandex.com/adv/products/display/mainpage#price
            permanent(h + '/price/media/homepage.xml', 'yandex.com/adv/products/display/mainpage#price'),

            # ^/price/media/(weather|maps|avia|realty|travel|afisha|music|afishamusic|lifestyle)\.xml$ - https://yandex.com/adv/products/display/$1#price
            # ^/price/media/weather\.xml$ - https://yandex.com/adv/products/display/weather#price
            permanent(h + '/price/media/weather.xml', 'yandex.com/adv/products/display/weather#price'),
            # ^/price/media/maps\.xml$ - https://yandex.com/adv/products/display/maps#price
            permanent(h + '/price/media/maps.xml', 'yandex.com/adv/products/display/maps#price'),
            # ^/price/media/avia\.xml$ - https://yandex.com/adv/products/display/avia#price
            permanent(h + '/price/media/avia.xml', 'yandex.com/adv/products/display/avia#price'),
            # ^/price/media/realty\.xml$ - https://yandex.com/adv/products/display/realty#price
            permanent(h + '/price/media/realty.xml', 'yandex.com/adv/products/display/realty#price'),
            # ^/price/media/travel\.xml$ - https://yandex.com/adv/products/display/travel#price
            permanent(h + '/price/media/travel.xml', 'yandex.com/adv/products/display/travel#price'),
            # ^/price/media/afisha\.xml$ - https://yandex.com/adv/products/display/afisha#price
            permanent(h + '/price/media/afisha.xml', 'yandex.com/adv/products/display/afisha#price'),
            # ^/price/media/music\.xml$ - https://yandex.com/adv/products/display/music#price
            permanent(h + '/price/media/music.xml', 'yandex.com/adv/products/display/music#price'),
            # ^/price/media/afishamusic\.xml$ - https://yandex.com/adv/products/display/afishamusic#price
            permanent(h + '/price/media/afishamusic.xml', 'yandex.com/adv/products/display/afishamusic#price'),
            # ^/price/media/lifestyle\.xml$ - https://yandex.com/adv/products/display/lifestyle#price
            permanent(h + '/price/media/lifestyle.xml', 'yandex.com/adv/products/display/lifestyle#price'),

            # ^/price/media/(marketbanner|market|timetables)\.xml$ - https://yandex.com/adv/products/display/marketmedia#price
            # ^/price/media/marketbanner\.xml$ - https://yandex.com/adv/products/display/marketmedia#price
            permanent(h + '/price/media/marketbanner.xml', 'yandex.com/adv/products/display/marketmedia#price'),
            # ^/price/media/market\.xml$ - https://yandex.com/adv/products/display/marketmedia#price
            permanent(h + '/price/media/market.xml', 'yandex.com/adv/products/display/marketmedia#price'),
            # ^/price/media/timetables\.xml$ - https://yandex.com/adv/products/display/marketmedia#price
            permanent(h + '/price/media/timetables.xml', 'yandex.com/adv/products/display/marketmedia#price'),

            # ^/price/media/look-alike\.xml$ - https://yandex.com/adv/products/display/videoweb#price
            permanent(h + '/price/media/look-alike.xml', 'yandex.com/adv/products/display/videoweb#price'),

            # ^/price/media/jobs\.xml$ - https://yandex.com/adv/products/display/rabota#price
            permanent(h + '/price/media/jobs.xml', 'yandex.com/adv/products/display/rabota#price'),

            # ^/price/media/kinopoisk\.xml$ - https://yandex.com/adv/products/display/kinopoiskmedia#price
            permanent(h + '/price/media/kinopoisk.xml', 'yandex.com/adv/products/display/kinopoiskmedia#price'),

            # ^/price/media/(auction|interests|retarget|network)\.xml$ - https://yandex.com/adv/products/display/auction#price
            # ^/price/media/auction\.xml$ - https://yandex.com/adv/products/display/auction#price
            permanent(h + '/price/media/auction.xml', 'yandex.com/adv/products/display/auction#price'),
            # ^/price/media/interests\.xml$ - https://yandex.com/adv/products/display/auction#price
            permanent(h + '/price/media/interests.xml', 'yandex.com/adv/products/display/auction#price'),
            # ^/price/media/retarget\.xml$ - https://yandex.com/adv/products/display/auction#price
            permanent(h + '/price/media/retarget.xml', 'yandex.com/adv/products/display/auction#price'),
            # ^/price/media/network\.xml$ - https://yandex.com/adv/products/display/auction#price
            permanent(h + '/price/media/network.xml', 'yandex.com/adv/products/display/auction#price'),

            # ^/price/media/video\.xml$ - https://yandex.com/adv/products/display/videobanner#price
            permanent(h + '/price/media/video.xml', 'yandex.com/adv/products/display/videobanner#price'),

            # ^/price/media/(package|look-alike)\.xml$ - https://yandex.com/adv/products/display/audience#price
            # ^/price/media/package\.xml$ - https://yandex.com/adv/products/display/audience#price
            permanent(h + '/price/media/package.xml', 'yandex.com/adv/products/display/audience#price'),
            # ^/price/media/look-alike\.xml$ - https://yandex.com/adv/products/display/audience#price
            # contradicts ^/price/media/look-alike\.xml$ - https://yandex.com/adv/products/display/videoweb#price

            # ^/price/media/(videoweb_yandex|videoweb_auction)\.xml$ - https://yandex.com/adv/products/display/videoweb#price
            # ^/price/media/videoweb_yandex\.xml$ - https://yandex.com/adv/products/display/videoweb#price
            permanent(h + '/price/media/videoweb_yandex.xml', 'yandex.com/adv/products/display/videoweb#price'),
            # ^/price/media/videoweb_auction\.xml$ - https://yandex.com/adv/products/display/videoweb#price
            permanent(h + '/price/media/videoweb_auction.xml', 'yandex.com/adv/products/display/videoweb#price'),

            # ^/price/media/crossmedia_network\.xml$ - https://yandex.com/adv/products/display/cross-media-network#price
            permanent(h + '/price/media/crossmedia_network.xml', 'yandex.com/adv/products/display/cross-media-network#price'),

            # ^/price/audio/(radio|radio_auction)\.xml$ - https://yandex.com/adv/products/display/audio#price
            # ^/price/audio/radio\.xml$ - https://yandex.com/adv/products/display/audio#price
            permanent(h + '/price/audio/radio.xml', 'yandex.com/adv/products/display/audio#price'),
            # ^/price/audio/radio_auction\.xml$ - https://yandex.com/adv/products/display/audio#price
            permanent(h + '/price/audio/radio_auction.xml', 'yandex.com/adv/products/display/audio#price'),

            # ^/price/media/contextual\.xml$ - https://yandex.com/adv/products/context/contextdisplay#price;
            permanent(h + '/price/media/contextual.xml', 'yandex.com/adv/products/context/contextdisplay#price'),

            # ^/price/media/(mobile_homepage|mobile_homepage_tablets)\.xml$ - https://yandex.com/adv/products/mobile/mobile-mainpage#price
            # ^/price/media/mobile_homepage\.xml$ - https://yandex.com/adv/products/mobile/mobile-mainpage#price
            permanent(h + '/price/media/mobile_homepage.xml', 'yandex.com/adv/products/mobile/mobile-mainpage#price'),
            # ^/price/media/mobile_homepage_tablets\.xml$ - https://yandex.com/adv/products/mobile/mobile-mainpage#price
            permanent(h + '/price/media/mobile_homepage_tablets.xml', 'yandex.com/adv/products/mobile/mobile-mainpage#price'),

            # ^/price/media/(auction_mobilegeo|mobile)\.xml$ - https://yandex.com/adv/products/mobile/mobileauction#price
            # ^/price/media/auction_mobilegeo\.xml$ - https://yandex.com/adv/products/mobile/mobileauction#price
            permanent(h + '/price/media/auction_mobilegeo.xml', 'yandex.com/adv/products/mobile/mobileauction#price'),
            # ^/price/media/mobile\.xml$ - https://yandex.com/adv/products/mobile/mobileauction#price
            permanent(h + '/price/media/mobile.xml', 'yandex.com/adv/products/mobile/mobileauction#price'),

            # ^/price/media/(.*) - https://yandex.com/adv/
            permanent(h + '/price/media/*', 'yandex.com/adv/'),

            # ^/price/discount/media\.xml$ - https://yandex.com/adv/
            permanent(h + '/price/discount/media.xml', 'yandex.com/adv/'),

            # ^/price/discount/(context-tr|context)\.xml$ - https://yandex.com/adv/discount-context
            # ^/price/discount/context-tr\.xml$ - https://yandex.com/adv/discount-context
            permanent(h + '/price/discount/context-tr.xml', 'yandex.com/adv/discount-context'),
            # ^/price/discount/context\.xml$ - https://yandex.com/adv/discount-context
            permanent(h + '/price/discount/context.xml', 'yandex.com/adv/discount-context'),

            # ^/price/priority/(reference|amenities)\.xml$ - https://yandex.com/adv/
            # ^/price/priority/reference\.xml$ - https://yandex.com/adv/
            permanent(h + '/price/priority/reference.xml', 'yandex.com/adv/'),
            # ^/price/priority/amenities\.xml$ - https://yandex.com/adv/
            permanent(h + '/price/priority/amenities.xml', 'yandex.com/adv/'),

            # ^/price/?$ - https://yandex.com/adv/prices
            permanent(h + '/price/', 'yandex.com/adv/prices'),

            # ^/product/settings/strategies\.xml$ - https://yandex.com/adv/products/materials/strategies
            permanent(h + '/product/settings/strategies.xml', 'yandex.com/adv/products/materials/strategies'),

            # ^/product/requirement/(flash|html5|kinopoisk)\.xml$ - https://yandex.com/adv/requirements/$1
            # ^/product/requirement/flash\.xml$ - https://yandex.com/adv/requirements/flash
            permanent(h + '/product/requirement/flash.xml', 'yandex.com/adv/requirements/flash'),
            # ^/product/requirement/html5\.xml$ - https://yandex.com/adv/requirements/html5
            permanent(h + '/product/requirement/html5.xml', 'yandex.com/adv/requirements/html5'),
            # ^/product/requirement/kinopoisk\.xml$ - https://yandex.com/adv/requirements/kinopoisk
            permanent(h + '/product/requirement/kinopoisk.xml', 'yandex.com/adv/requirements/kinopoisk'),

            # ^/product/requirement/main\.xml$ - https://yandex.com/adv/requirements/mainpage
            permanent(h + '/product/requirement/main.xml', 'yandex.com/adv/requirements/mainpage'),

            # ^/product/requirement/videoweb\.xml$ - https://yandex.com/adv/requirements/videowebreq
            permanent(h + '/product/requirement/videoweb.xml', 'yandex.com/adv/requirements/videowebreq'),

            # ^/product/requirement/auction_mobileweb\.xml$ - https://yandex.com/adv/requirements/mobileauctionreq
            permanent(h + '/product/requirement/auction_mobileweb.xml', 'yandex.com/adv/requirements/mobileauctionreq'),

            # ^/product/requirement/audio/radio\.xml$ - https://yandex.com/adv/requirements/audioreq
            permanent(h + '/product/requirement/audio/radio.xml', 'yandex.com/adv/requirements/audioreq'),

            # ^/product/requirement/crossmedia_network\.xml$ - https://yandex.com/adv/requirements/cross-media
            permanent(h + '/product/requirement/crossmedia_network.xml', 'yandex.com/adv/requirements/cross-media'),

            # ^/product/(.*) - https://yandex.com/adv/
            permanent(h + '/product/*', 'yandex.com/adv/'),

            # ^/agency/(faq\.xml)? - https://yandex.com/adv/partners/
            # ^/agency/ - https://yandex.com/adv/partners/
            permanent(h + '/agency/', 'yandex.com/adv/partners/'),
            # ^/agency/faq\.xml - https://yandex.com/adv/partners/
            permanent(h + '/agency/faq.xml', 'yandex.com/adv/partners/'),

            # ^/contact/?$ - https://yandex.com/adv/contact/
            permanent(h + '/contact/', 'yandex.com/adv/contact/'),

            # ^/contact/agency/(.*) - https://yandex.com/adv/contact/agency
            permanent(h + '/contact/agency/*', 'yandex.com/adv/contact/agency'),

            # ^/contact/yandex/order\.xml$ - https://yandex.com/adv/order/context/placead
            permanent(h + '/contact/yandex/order.xml', 'yandex.com/adv/order/context/placead'),

            # ^/contact/yandex/support\.xml$ - https://yandex.com/adv/contact/
            permanent(h + '/contact/yandex/support.xml', 'yandex.com/adv/contact/'),

            # ^/contact/yandex/sale\.xml$ - https://yandex.com/adv/contact/offices
            permanent(h + '/contact/yandex/sale.xml', 'yandex.com/adv/contact/offices'),

            # ^/education/ - https://yandex.com/adv/edu/
            permanent(h + '/education/', 'yandex.com/adv/edu/'),
        ]

    for h in [
        str('advertising.yandex.com.tr'),
        str('www.advertising.yandex.com.tr'),
    ]:
        res += [
            # ^/news/(.*) - https://yandex.com.tr/adv/news
            permanent(h + '/news/*', 'yandex.com.tr/adv/news'),

            # ^/product/context/direct\.xml$ - https://yandex.com.tr/adv/products/context/search
            permanent(h + '/product/context/direct.xml', 'yandex.com.tr/adv/products/context/search'),

            # ^/(context|product)/direct\.xml$ - https://yandex.com.tr/adv/products/context/search
            # ^/context/direct\.xml$ - https://yandex.com.tr/adv/products/context/search
            permanent(h + '/context/direct.xml', 'yandex.com.tr/adv/products/context/search'),
            # ^/product/direct\.xml$ - https://yandex.com.tr/adv/products/context/search
            permanent(h + '/product/direct.xml', 'yandex.com.tr/adv/products/context/search'),

            # ^/context/((market|metrica)\.xml)?$ - https://yandex.com.tr/adv/
            # ^/context/$ - https://yandex.com.tr/adv/
            permanent(h + '/context/', 'yandex.com.tr/adv/'),
            # ^/context/market\.xml$ - https://yandex.com.tr/adv/
            permanent(h + '/context/market.xml', 'yandex.com.tr/adv/'),
            # ^/context/metrica\.xml$ - https://yandex.com.tr/adv/
            permanent(h + '/context/metrica.xml', 'yandex.com.tr/adv/'),

            # ^/media/(homepage\.xml)?$ - https://yandex.com.tr/adv/
            # ^/media/$ - https://yandex.com.tr/adv/
            permanent(h + '/media/', 'yandex.com.tr/adv/'),
            # ^/media/homepage\.xml$ - https://yandex.com.tr/adv/
            permanent(h + '/media/homepage.xml', 'yandex.com.tr/adv/'),

            # ^/product/(homepage|portal)\.xml$ - https://yandex.com.tr/adv/
            # ^/product/homepage\.xml$ - https://yandex.com.tr/adv/
            permanent(h + '/product/homepage.xml', 'yandex.com.tr/adv/'),
            # ^/product/portal\.xml$ - https://yandex.com.tr/adv/
            permanent(h + '/product/portal.xml', 'yandex.com.tr/adv/'),

            # ^/product/media/maps\.xml$ - https://yandex.com.tr/adv/products/display
            permanent(h + '/product/media/maps.xml', 'yandex.com.tr/adv/products/display'),

            # ^/product/media/(contextual|contextual-packages)\.xml$ - https://yandex.com.tr/adv/products/context/contextdisplay
            # ^/product/media/contextual\.xml$ - https://yandex.com.tr/adv/products/context/contextdisplay
            permanent(h + '/product/media/contextual.xml', 'yandex.com.tr/adv/products/context/contextdisplay'),
            # ^/product/media/contextual-packages\.xml$ - https://yandex.com.tr/adv/products/context/contextdisplay
            permanent(h + '/product/media/contextual-packages.xml', 'yandex.com.tr/adv/products/context/contextdisplay'),

            # ^/product/media/(.*) - https://yandex.com.tr/adv/
            permanent(h + '/product/media/*', 'yandex.com.tr/adv/'),

            # ^/price(/|\.xml)?$ - https://yandex.com.tr/adv/prices
            # ^/price/$ - https://yandex.com.tr/adv/prices
            permanent(h + '/price/', 'yandex.com.tr/adv/prices'),
            # ^/price\.xml$ - https://yandex.com.tr/adv/prices
            permanent(h + '/price.xml', 'yandex.com.tr/adv/prices'),

            # ^/price/(context/)?direct\.xml$ - https://yandex.com.tr/adv/products/context/search#price
            # ^/price/context/direct\.xml$ - https://yandex.com.tr/adv/products/context/search#price
            permanent(h + '/price/context/direct.xml', 'yandex.com.tr/adv/products/context/search#price'),
            # ^/price/direct\.xml$ - https://yandex.com.tr/adv/products/context/search#price
            permanent(h + '/price/direct.xml', 'yandex.com.tr/adv/products/context/search#price'),

            # ^/price/media/maps\.xml$ - https://yandex.com.tr/adv/products/display/maps
            permanent(h + '/price/media/maps.xml', 'yandex.com.tr/adv/products/display/maps'),

            # ^/price/media/contextual\.xml$ - https://yandex.com.tr/adv/products/context/contextdisplay#price
            permanent(h + '/price/media/contextual.xml', 'yandex.com.tr/adv/products/context/contextdisplay#price'),

            # ^/price/discount/context\.xml$ - https://yandex.com.tr/adv/discount-context
            permanent(h + '/price/discount/context.xml', 'yandex.com.tr/adv/discount-context'),

            # ^/price/(.*) - https://yandex.com.tr/adv/
            permanent(h + '/price/*', 'yandex.com.tr/adv/'),

            # ^/contact/?$ - https://yandex.com.tr/adv/contact/
            permanent(h + '/contact/', 'yandex.com.tr/adv/contact/'),

            # ^/contact/agency/(partner\.xml)?$ - https://yandex.com.tr/adv/contact/agency?cities[]=11508
            # ^/contact/agency/$ - https://yandex.com.tr/adv/contact/agency?cities[]=11508
            permanent(h + '/contact/agency/', 'yandex.com.tr/adv/contact/agency?cities[]=11508'),
            # ^/contact/agency/partner\.xml$ - https://yandex.com.tr/adv/contact/agency?cities[]=11508
            permanent(h + '/contact/agency/partner.xml', 'yandex.com.tr/adv/contact/agency?cities[]=11508'),

            # ^/contact/order\.xml$ - https://yandex.com.tr/adv/order/context/placead
            permanent(h + '/contact/order.xml', 'yandex.com.tr/adv/order/context/placead'),

            # ^/requirement/(flash|html5)\.xml$ - https://yandex.com.tr/adv/requirements/$1
            # ^/requirement/flash\.xml$ - https://yandex.com.tr/adv/requirements/flash
            permanent(h + '/requirement/flash.xml', 'yandex.com.tr/adv/requirements/flash'),
            # ^/requirement/html5\.xml$ - https://yandex.com.tr/adv/requirements/html5
            permanent(h + '/requirement/html5.xml', 'yandex.com.tr/adv/requirements/html5'),

            # ^/agency/(.*) - https://yandex.com.tr/adv/contact/agency?cities[]=11508
            permanent(h + '/agency/*', 'yandex.com.tr/adv/contact/agency?cities[]=11508'),

            # ^/product/?$ - https://yandex.com.tr/adv/
            permanent(h + '/product/', 'yandex.com.tr/adv/'),
        ]

    for h in [
        str('adv.yandex.ru'),
        str('www.adv.yandex.ru'),
        str('advertising.yandex.ru'),
        str('www.advertising.yandex.ru'),
        str('reklama.yandex.ru'),
        str('www.reklama.yandex.ru'),
    ]:
        for src in [
            h + '/i/status/certificate_big.png',
            h + '/i/status/certificate_direct_2016.png',
        ]:
            res += [
                permanent(src, 'avatars.mds.yandex.net/get-adv/135342/2a0000015456e6c2ea20942865751614b9c5/a_certif'),
            ]

    res += [
        permanent('adv.yandex.ru/welcome/?id=bing', 'direct.yandex.ru/'),
        permanent('advertising.yandex.ru/welcome/?id=bing', 'direct.yandex.ru/'),
        permanent('www.adv.yandex.ru/welcome/?id=bing', 'direct.yandex.ru/'),
        permanent('www.advertising.yandex.ru/welcome/?id=bing', 'direct.yandex.ru/'),
    ]

    return res
