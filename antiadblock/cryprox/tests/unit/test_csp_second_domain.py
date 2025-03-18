import pytest

from antiadblock.cryprox.cryprox.common.tools.misc import add_second_domain_to_csp


BODY_TEMPLATE = '<meta http-equiv="Content-Security-Policy" content="{}">'


@pytest.mark.parametrize(
    'csp,expected_csp',
    [
        ("default-src; script-src 'self';", "default-src; script-src 'self' test.com;"),
        ("default-src 'none'; script-src something.com;", "default-src 'none'; script-src something.com test.com;"),
        (
            "default-src 'self'; script-src 'report-sample' 'self' *.speedcurve.com "
            "'sha256-q7cJjDqNO2e1L5UltvJ1LhvnYN7yJXgGO7b6h9xkL1o=' www.google-analytics.com/analytics.js "
            "'sha256-JEt9Nmc3BP88wxuTZm9aKNu87vEgGmKW1zzy/vb1KPs=' polyfill.io/v3/polyfill.min.js assets.codepen.io "
            "production-assets.codepen.io; script-src-elem 'report-sample' 'self' *.speedcurve.com "
            "'sha256-q7cJjDqNO2e1L5UltvJ1LhvnYN7yJXgGO7b6h9xkL1o=' www.google-analytics.com/analytics.js "
            "'sha256-JEt9Nmc3BP88wxuTZm9aKNu87vEgGmKW1zzy/vb1KPs=' polyfill.io/v3/polyfill.min.js assets.codepen.io "
            "production-assets.codepen.io; style-src 'report-sample' 'self' 'unsafe-inline'; object-src 'none'; "
            "base-uri 'self'; connect-src 'self' www.google-analytics.com stats.g.doubleclick.net; font-src 'self'; "
            "frame-src 'self' interactive-examples.mdn.mozilla.net mdn.github.io yari-demos.prod.mdn.mozit.cloud "
            "mdn.mozillademos.org yari-demos.stage.mdn.mozit.cloud jsfiddle.net www.youtube-nocookie.com codepen.io; "
            "img-src 'self' *.githubusercontent.com *.googleusercontent.com lux.speedcurve.com mdn.mozillademos.org "
            "media.prod.mdn.mozit.cloud media.stage.mdn.mozit.cloud interactive-examples.mdn.mozilla.net "
            "wikipedia.org www.google-analytics.com www.gstatic.com; manifest-src 'self'; media-src 'self' "
            "archive.org videos.cdn.mozilla.net; worker-src 'none';",

            "default-src 'self' test.com; script-src 'report-sample' 'self' *.speedcurve.com "
            "'sha256-q7cJjDqNO2e1L5UltvJ1LhvnYN7yJXgGO7b6h9xkL1o=' www.google-analytics.com/analytics.js "
            "'sha256-JEt9Nmc3BP88wxuTZm9aKNu87vEgGmKW1zzy/vb1KPs=' polyfill.io/v3/polyfill.min.js assets.codepen.io "
            "production-assets.codepen.io test.com; script-src-elem 'report-sample' 'self' *.speedcurve.com "
            "'sha256-q7cJjDqNO2e1L5UltvJ1LhvnYN7yJXgGO7b6h9xkL1o=' www.google-analytics.com/analytics.js "
            "'sha256-JEt9Nmc3BP88wxuTZm9aKNu87vEgGmKW1zzy/vb1KPs=' polyfill.io/v3/polyfill.min.js assets.codepen.io "
            "production-assets.codepen.io; style-src 'report-sample' 'self' 'unsafe-inline' test.com; "
            "object-src 'none'; base-uri 'self'; connect-src 'self' www.google-analytics.com "
            "stats.g.doubleclick.net test.com; font-src 'self' test.com; frame-src 'self' "
            "interactive-examples.mdn.mozilla.net mdn.github.io yari-demos.prod.mdn.mozit.cloud mdn.mozillademos.org "
            "yari-demos.stage.mdn.mozit.cloud jsfiddle.net www.youtube-nocookie.com codepen.io test.com; img-src "
            "'self' *.githubusercontent.com *.googleusercontent.com lux.speedcurve.com mdn.mozillademos.org "
            "media.prod.mdn.mozit.cloud media.stage.mdn.mozit.cloud interactive-examples.mdn.mozilla.net "
            "wikipedia.org www.google-analytics.com www.gstatic.com test.com; manifest-src 'self' test.com; "
            "media-src 'self' archive.org videos.cdn.mozilla.net test.com; worker-src 'none';"
        ),
        (
            "default-src 'none'; base-uri 'self'; block-all-mixed-content; child-src github.com/assets-cdn/worker/ "
            "gist.github.com/assets-cdn/worker/; connect-src 'self' uploads.github.com "
            "objects-origin.githubusercontent.com www.githubstatus.com collector.githubapp.com api.github.com "
            "github-cloud.s3.amazonaws.com github-production-repository-file-5c1aeb.s3.amazonaws.com "
            "github-production-upload-manifest-file-7fdce7.s3.amazonaws.com "
            "github-production-user-asset-6210df.s3.amazonaws.com cdn.optimizely.com logx.optimizely.com/v1/events "
            "translator.github.com wss://alive.github.com github.githubassets.com; font-src github.githubassets.com; "
            "form-action 'self' github.com gist.github.com objects-origin.githubusercontent.com; frame-ancestors "
            "'none'; frame-src render.githubusercontent.com viewscreen.githubusercontent.com "
            "notebooks.githubusercontent.com; img-src 'self' data: github.githubassets.com identicons.github.com "
            "collector.githubapp.com github-cloud.s3.amazonaws.com secured-user-images.githubusercontent.com/ "
            "*.githubusercontent.com customer-stories-feed.github.com spotlights-feed.github.com; manifest-src "
            "'self'; media-src github.com user-images.githubusercontent.com/ github.githubassets.com; script-src "
            "github.githubassets.com; style-src 'unsafe-inline' github.githubassets.com; worker-src "
            "github.com/assets-cdn/worker/ gist.github.com/assets-cdn/worker/",

            "default-src 'none'; base-uri 'self'; block-all-mixed-content; child-src "
            "github.com/assets-cdn/worker/ gist.github.com/assets-cdn/worker/ test.com; connect-src 'self' "
            "uploads.github.com objects-origin.githubusercontent.com www.githubstatus.com collector.githubapp.com "
            "api.github.com github-cloud.s3.amazonaws.com github-production-repository-file-5c1aeb.s3.amazonaws.com "
            "github-production-upload-manifest-file-7fdce7.s3.amazonaws.com "
            "github-production-user-asset-6210df.s3.amazonaws.com cdn.optimizely.com logx.optimizely.com/v1/events "
            "translator.github.com wss://alive.github.com github.githubassets.com test.com; font-src "
            "github.githubassets.com test.com; form-action 'self' github.com gist.github.com "
            "objects-origin.githubusercontent.com; frame-ancestors 'none'; frame-src render.githubusercontent.com "
            "viewscreen.githubusercontent.com notebooks.githubusercontent.com test.com; img-src 'self' data: "
            "github.githubassets.com identicons.github.com collector.githubapp.com github-cloud.s3.amazonaws.com "
            "secured-user-images.githubusercontent.com/ *.githubusercontent.com customer-stories-feed.github.com "
            "spotlights-feed.github.com test.com; manifest-src 'self' test.com; media-src github.com "
            "user-images.githubusercontent.com/ github.githubassets.com test.com; script-src github.githubassets.com "
            "test.com; style-src 'unsafe-inline' github.githubassets.com test.com; worker-src "
            "github.com/assets-cdn/worker/ gist.github.com/assets-cdn/worker/ test.com"
        ),
        (
            "frame-src https://passport.yandex.ru https://yandex.ru https://yastatic.net 'self' "
            "https://storage.mds.yandex.net https://forms.yandex.ru https://mc.yandex.ru https://mc.yandex.md "
            "https://passport.yandex.ru;connect-src blob: https://*.cdn.ngenix.net https://*.strm.yandex.net "
            "https://auto.ru https://favicon.yandex.net https://log.strm.yandex.ru https://mc.yandex.com "
            "https://thequestion.ru https://www.kinopoisk.ru https://zen-yandex-ru.cdnclab.net https://yandex.ru "
            "https://yastatic.net https://yastat.net 'self' https://portal-xiva.yandex.net "
            "wss://portal-xiva.yandex.net https://strm.yandex.ru https://mobile.yandex.net https://yabs.yandex.ru "
            "https://an.yandex.ru https://verify.yandex.ru https://*.verify.yandex.ru https://mc.yandex.ru "
            "https://yandex.st https://matchid.adfox.yandex.ru https://adfox.yandex.ru https://ads.adfox.ru "
            "https://ads6.adfox.ru https://jstracer.yandex.ru https://awaps.yandex.ru https://tps.doubleverify.com "
            "https://pixel.adsafeprotected.com https://*.mc.yandex.ru https://adstat.yandex.ru "
            "https://mc.admetrica.ru;media-src blob: https://*.cdn.ngenix.net https://*.strm.yandex.net "
            "https://*.yandex.net https://strm.yandex.ru https://*.strm.yandex.ru https://yastat.net data:;script-src "
            "'unsafe-inline' https://mc.yandex.com https://zen-yandex-ru.cdnclab.net https://yastatic.net "
            "https://yandex.ru 'self' https://an.yandex.ru https://z.moatads.com https://storage.mds.yandex.net "
            "https://mc.yandex.ru https://*.mc.yandex.ru https://adstat.yandex.ru;report-uri "
            "https://csp.yandex.net/csp?project=morda&from=morda.big.ru&showid=1638447254.6772.82754.118572&h=stable"
            "-morda-sas-yp-46&yandexuid=8969502341638447254&&version=2021-12-01-1&adb=0;style-src 'unsafe-inline' "
            "https://yastatic.net;img-src https://*.verify.yandex.ru https://auto.ru https://strm.yandex.net "
            "https://thequestion.ru https://www.kinopoisk.ru https://zen-yandex-ru.cdnclab.net 'self' "
            "https://yastatic.net data: https://yandex.ru https://resize.yandex.net https://*.strm.yandex.net "
            "https://strm.yandex.ru https://avatars-fast.yandex.net https://favicon.yandex.net "
            "https://banners.adfox.ru https://content.adfox.ru https://ads6.adfox.ru https://yastat.net "
            "https://avatars.mds.yandex.net https://mc.yandex.ru https://*.tns-counter.ru https://verify.yandex.ru "
            "https://ads.adfox.ru https://bs.serving-sys.com https://ad.adriver.ru https://wcm.solution.weborama.fr "
            "https://wcm-ru.frontend.weborama.fr https://mc.admetrica.ru https://ad.doubleclick.net https://rgi.io "
            "https://track.rutarget.ru https://ssl.hurra.com https://px.moatads.com https://amc.yandex.ru "
            "https://gdeby.hit.gemius.pl https://tps.doubleverify.com https://pixel.adsafeprotected.com "
            "https://impression.appsflyer.com https://an.yandex.ru https://storage.mds.yandex.net "
            "https://yabs.yandex.ru https://mc.yandex.com https://*.mc.yandex.ru https://adstat.yandex.ru;default-src "
            "https://yastatic.net https://yastat.net;font-src https://yastatic.net;object-src "
            "https://avatars.mds.yandex.net",

            "frame-src https://passport.yandex.ru https://yandex.ru https://yastatic.net 'self' "
            "https://storage.mds.yandex.net https://forms.yandex.ru https://mc.yandex.ru https://mc.yandex.md "
            "https://passport.yandex.ru test.com;connect-src blob: https://*.cdn.ngenix.net https://*.strm.yandex.net "
            "https://auto.ru https://favicon.yandex.net https://log.strm.yandex.ru https://mc.yandex.com "
            "https://thequestion.ru https://www.kinopoisk.ru https://zen-yandex-ru.cdnclab.net https://yandex.ru "
            "https://yastatic.net https://yastat.net 'self' https://portal-xiva.yandex.net "
            "wss://portal-xiva.yandex.net https://strm.yandex.ru https://mobile.yandex.net https://yabs.yandex.ru "
            "https://an.yandex.ru https://verify.yandex.ru https://*.verify.yandex.ru https://mc.yandex.ru "
            "https://yandex.st https://matchid.adfox.yandex.ru https://adfox.yandex.ru https://ads.adfox.ru "
            "https://ads6.adfox.ru https://jstracer.yandex.ru https://awaps.yandex.ru https://tps.doubleverify.com "
            "https://pixel.adsafeprotected.com https://*.mc.yandex.ru https://adstat.yandex.ru "
            "https://mc.admetrica.ru test.com;media-src blob: https://*.cdn.ngenix.net https://*.strm.yandex.net "
            "https://*.yandex.net https://strm.yandex.ru https://*.strm.yandex.ru https://yastat.net data: "
            "test.com;script-src 'unsafe-inline' https://mc.yandex.com https://zen-yandex-ru.cdnclab.net "
            "https://yastatic.net https://yandex.ru 'self' https://an.yandex.ru https://z.moatads.com "
            "https://storage.mds.yandex.net https://mc.yandex.ru https://*.mc.yandex.ru https://adstat.yandex.ru "
            "test.com;report-uri https://csp.yandex.net/csp?project=morda&from=morda.big.ru&showid=1638447254.6772"
            ".82754.118572&h=stable-morda-sas-yp-46&yandexuid=8969502341638447254&&version=2021-12-01-1&adb=0;style"
            "-src 'unsafe-inline' https://yastatic.net test.com;img-src https://*.verify.yandex.ru https://auto.ru "
            "https://strm.yandex.net https://thequestion.ru https://www.kinopoisk.ru "
            "https://zen-yandex-ru.cdnclab.net 'self' https://yastatic.net data: https://yandex.ru "
            "https://resize.yandex.net https://*.strm.yandex.net https://strm.yandex.ru "
            "https://avatars-fast.yandex.net https://favicon.yandex.net https://banners.adfox.ru "
            "https://content.adfox.ru https://ads6.adfox.ru https://yastat.net https://avatars.mds.yandex.net "
            "https://mc.yandex.ru https://*.tns-counter.ru https://verify.yandex.ru https://ads.adfox.ru "
            "https://bs.serving-sys.com https://ad.adriver.ru https://wcm.solution.weborama.fr "
            "https://wcm-ru.frontend.weborama.fr https://mc.admetrica.ru https://ad.doubleclick.net https://rgi.io "
            "https://track.rutarget.ru https://ssl.hurra.com https://px.moatads.com https://amc.yandex.ru "
            "https://gdeby.hit.gemius.pl https://tps.doubleverify.com https://pixel.adsafeprotected.com "
            "https://impression.appsflyer.com https://an.yandex.ru https://storage.mds.yandex.net "
            "https://yabs.yandex.ru https://mc.yandex.com https://*.mc.yandex.ru https://adstat.yandex.ru "
            "test.com;default-src https://yastatic.net https://yastat.net test.com;font-src https://yastatic.net "
            "test.com;object-src https://avatars.mds.yandex.net test.com"
        ),
    ],
)
def test_second_domain_addition_to_csp(csp, expected_csp):
    headers = {'Content-Security-Policy': csp}
    body = BODY_TEMPLATE.format(csp)
    domain = 'test.com'

    result_body = add_second_domain_to_csp(headers, body, domain)

    assert result_body == BODY_TEMPLATE.format(expected_csp)
