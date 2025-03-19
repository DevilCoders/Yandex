hosts:
  hd: testing-qloud.hd.kinopoisk.ru
  frontend: kp-front-balancer.tst.kp.yandex.net
  backend: kp-back-balancer.tst.kp.yandex.net
  backend_ext: kp-mobile-api.tst.kp.yandex.net
  master: bo.tst.kinopoisk.ru
  static: st.tst.kp.yandex.net
  memcache: kp-memcache.tst.kp.yandex.net
  msess: kp-msess.tst.kp.yandex.net
  antiadblock: cryprox-test.yandex.net:80
  auth: auth-app.testing.kinopoisk.ru
  touch: testing.touch.kinopoisk.ru
  mds: storage.mdst.yandex.net
  s3: s3.mdst.yandex.net
  s3_kinopoisk: kinopoisk.s3.mdst.yandex.net
  s3_static: kinopoisk-static.s3.mdst.yandex.net
  s3_mobile_promo: kinopoisk-mobile.s3.mdst.yandex.net
  avatars: avatars.mdst.yandex.net
  kpp: plus.tst.kinopoisk.ru
  desktop_frontend: desktop.tst.kinopoisk.ru
  special_frontend: special.testing.kinopoisk.ru
  mobileadapter: ma.tst.kp.yandex.net
  ott_device: ott-device.testing.kinopoisk.ru
  ott_promo: ott-promo.testing.kinopoisk.ru
  yandex: yandex.ru
  kp1_api: kp1-api.tst.kp.yandex.net
  kp1_api_qloud: kp1-api-qloud.tst.kp.yandex.net
  guess_game: testing.guess-game.outsource.kinopoisk.ru
  accel_redirect: kinopoisk-accel-redirect-testing.stable.qloud-b.yandex.net
  oscar_kinopoisk: oscar.testing.kinopoisk.ru
  auth_app: kp-auth.tst.kp.yandex.net
  angel: kinopoisk-angel-testing.stable.qloud-b.yandex.net
  sphinx: kp-heavy-dbs.tst.kp.yandex.net

domains:
  hd: testing.hd.kinopoisk.ru
  kp_web: testing.kinopoisk.ru
  kp_ext: ext.tst.kinopoisk.ru
  kp_touch_api: touch-api.tst.kinopoisk.ru
  kp_rating: rating.tst.kinopoisk.ru
  kp_reg: reg.tst.kinopoisk.ru
  kp_static: st.tst.kp.yandex.net
  kp_bo: bo.tst.kinopoisk.ru
  auth: auth.tst.kinopoisk.ru
  yastatic: betastatic.yandex.net
  kp_api_internal: kp1-api-internal.tst.kp.yandex.net

hosts_ports:
  accel_redirect: 80
