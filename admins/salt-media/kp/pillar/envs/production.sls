hosts:
  hd: production-qloud.hd.kinopoisk.ru
  frontend: front.kp.yandex.net
  backend: bk.kp.yandex.net
  backend_ext: kp-mobile-api-balancer.kp.yandex.net
  master: bo.kinopoisk.ru
  static: st.kp.yandex.net
  memcache: mc.kp.yandex.net
  msess: kp-msess-balancer.kp.yandex.net
  antiadblock: cryprox.yandex.net
  auth: auth-app.kinopoisk.ru
  touch: production.touch.kinopoisk.ru
  mds: storage.mds.yandex.net
  s3: s3.mds.yandex.net
  s3_static: kinopoisk-static.s3.yandex.net
  s3_kinopoisk: kinopoisk.s3.mds.yandex.net
  s3_mobile_promo: kinopoisk-mobile.s3.mds.yandex.net
  avatars: avatars.mds.yandex.net
  kpp: plus.kinopoisk.ru
  desktop_frontend: desktop.kinopoisk.ru
  special_frontend: special.kinopoisk.ru
  mobileadapter: ma.kp.yandex.net
  ott_device: ott-device.kinopoisk.ru
  ott_promo: ott-promo.kinopoisk.ru
  yandex: yandex.ru
  kp1_api: kp1-api.kp.yandex.net
  kp1_api_qloud: kp1-api-qloud.kp.yandex.net
  guess_game: guess-game.outsource-prod.kinopoisk.ru
  accel_redirect: kinopoisk-accel-redirect.stable.qloud-b.yandex.net
  oscar_kinopoisk: oscar.kinopoisk.ru
  auth_app: kp-auth.kp.yandex.net
  angel: kinopoisk-angel-stable.stable.qloud-b.yandex.net
  sphinx: kp-heavy-dbs.kp.yandex.net

domains:
  hd: hd.kinopoisk.ru
  kp_web: www.kinopoisk.ru
  kp_ext: ext.kinopoisk.ru
  kp_touch_api: touch-api.kinopoisk.ru
  kp_rating: rating.kinopoisk.ru
  kp_reg: reg.kinopoisk.ru
  kp_static: st.kp.yandex.net
  kp_bo: bo.kinopoisk.ru
  auth: auth.kinopoisk.ru
  yastatic: yastatic.net
  kp_api_internal: kp1-api-internal.kp.yandex.net

hosts_ports:
  accel_redirect: 80
