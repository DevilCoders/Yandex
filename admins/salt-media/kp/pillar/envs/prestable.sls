hosts:
  hd: prestable-qloud.hd.kinopoisk.ru
  frontend: kp-front01h.prestable.kp.yandex.net
  backend: kp-back-balancer.prestable.kp.yandex.net
  backend_ext: kp-mobile-api.prestable.kp.yandex.net
  master: kp-back-balancer.prestable.kp.yandex.net
  static: st.prestable.kp.yandex.net
  memcache: kp-memcache.prestable.kp.yandex.net
  msess: kp-msess.prestable.kp.yandex.net
  antiadblock: cryprox-test.yandex.net:80
  auth: auth-app.prestable.kinopoisk.ru
  touch: prestable.touch.kinopoisk.ru
  mds: storage.mds.yandex.net
  s3: s3.mds.yandex.net
  s3_static: kinopoisk-static.s3.yandex.net
  s3_mobile_promo: kinopoisk-mobile.s3.mds.yandex.net
  s3_kinopoisk: kinopoisk.s3.mds.yandex.net
  avatars: avatars.mds.yandex.net
  kpp: plus.prestable.kinopoisk.ru
  desktop_frontend: desktop.prestable.kinopoisk.ru
  special_frontend: special.prestable.kinopoisk.ru
  mobileadapter: ma.prestable.kp.yandex.net
  ott_device: ott-device.kinopoisk.ru
  ott_promo: ott-promo.kinopoisk.ru
  yandex: yandex.ru
  kp1_api: kp1-api.prestable.kp.yandex.net
  kp1_api_qloud: kp1-api-qloud.prestable.kp.yandex.net
  guess_game: prestable.guess-game.outsource-prod.kinopoisk.ru
  accel_redirect: kinopoisk-accel-redirect-prestable.stable.qloud-b.yandex.net
  oscar_kinopoisk: oscar.prestable.kinopoisk.ru
  auth_app: kp-auth.kp.yandex.net
  angel: kinopoisk-angel-prestable.stable.qloud-b.yandex.net
  sphinx: kp-heavy-dbs.kp.yandex.net

domains:
  hd: prestable.hd.kinopoisk.ru
  kp_web: prestable.kinopoisk.ru
  kp_ext: ext.prestable.kinopoisk.ru
  kp_touch_api: touch-api.prestable.kinopoisk.ru
  kp_rating: rating.prestable.kinopoisk.ru
  kp_static: st01h.pre.kp.yandex.net
  kp_static_prod: st.kp.yandex.net
  kp_reg: reg.prestable.kinopoisk.ru
  kp_bo: bo.kinopoisk.ru
  auth: auth.prestable.kinopoisk.ru
  yastatic: yastatic.net
  kp_api_internal: kp1-api-internal.kp.yandex.net

hosts_ports:
  accel_redirect: 80
