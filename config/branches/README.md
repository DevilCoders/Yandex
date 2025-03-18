# Release branches
Пользователь Аркадии может создать бранчи с фиксированным префиксом в названии.
`releases/<названиве проекта>/<название бранча>`

Конфигурация автосборки в бранчах описывается в файлах `config/branches/<branch_name_prefix>.yaml`
Если в названии бранча есть символ `/`, то нужно создать поддиректорию в config/branches и положить файл туда.

CI берет содержимое конфига из транка, следит за появлением новых бранчей и коммитов в бранчи и изменением самого конфига.
Автоматически запускает тесты в бранчах.

Квота будет выделяться в общем пуле автосборки, раздельный заказ квот организовать дороже, чем заказать общую квоту на отдел.

Пример конфига:
```
--- 
arcanum: 
  auto_merge: 
    requirements: 
      data: 
        ignore_self_ship: true
        min_approvers_count: 1
      system: arcanum
      type: st_issue_linked
autocheck: 
  automute: 
    mute-trunk-flaky-tests: true
  platforms: 
    - default-linux-x86_64-relwithdebinfo
    - default-linux-x86_64-release-jdk15
  project-dirs: 
    - ci/registry
    - testenv/jobs
ci: 
  runtime: 
    sandbox-owner: TESTENGINE
  secret: sec-01etdcbs95qbpr8e07beyf3nkp
  strong-mode: 
    skip-strong-paths: 
      - arcadia/util
      - arcadia/build
      - arcadia/sandbox/projects
    strict-mode-groups: 
      - yandex_monetize_market_marketdev_business_dep66597_dep50644
      - yandex_monetize_market_marketdev_shipping
services: 
  - yt
```
