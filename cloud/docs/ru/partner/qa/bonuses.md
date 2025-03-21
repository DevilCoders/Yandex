# Схема премирования партнеров

#### Что такое партнерская премия? {#bonus}

Партнерская премия — это скидка на услуги {{ yandex-cloud }}, которая применяется к счету, выставленному на биллинг-аккаунт партнера {{ yandex-cloud }} по итогам месяца. В акте, счете и счете-фактуре для партнера указывается финальная сумма со скидкой.

#### От чего зависят размер скидки, порядок ее начисления, основания для расчета? {#discount}

Размер скидки зависит от суммарного месячного объема потребления услуг {{ yandex-cloud }} всеми [саб-аккаунтами](start-grant.md#sub-account), закрепленными за партнерским биллинг-аккаунтом.

Закрепить саб-аккаунт клиента за партнером можно в [партнерском портале](https://partners.cloud.yandex.ru/). Закрепление является основанием для начисления премии. Закрепление отражается в партнерском портале, в консоли управления {{ yandex-cloud }} партнера и в консоли управления закрепленного клиента.

Партнерская скидка не видна закрепленным саб-аккаунтам в консоли управления. Счета от {{ yandex-cloud }} на саб-аккаунты отдельно не выставляются.

Подробнее про размер скидки см. [Партнерское соглашение]{% if lang == "ru" %}(https://yandex.ru/legal/cloud_grant/?lang=ru){% endif %}{% if lang == "en" %}(https://yandex.ru/legal/cloud_grant/?lang=en){% endif %}.

#### Что не учитывается при расчете партнерской премии? {#no-bonus}

Скидка не распространяется на:
* объем потребления на аккаунте партнера;
* гранты, выданные на саб-аккаунты;
* ресурсы из {{ marketplace-name }} {{ yandex-cloud }}.

Абсолютный и относительный размер скидки за отчетный месяц можно увидеть в партнерском портале в разделе **Вознаграждения**.

#### Почему при расчете партнерской премии не учитываются ресурсы из {{ marketplace-name }} {{ yandex-cloud }}? {#marketplace}

{{ yandex-cloud }} продает услуги {{ marketplace-name }} с минимальной наценкой или без нее. Поэтому мы не можем дать дополнительную скидку на решения из образов {{ marketplace-name }}.
