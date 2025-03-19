external_config:
{% if salt["grains.get"]("conductor:group") in ["jkp-dev-back"] %}
{% set ec_sec_name = "sec-01d5tz2xv7y6r552t2d8g3cpea" %}
{% elif salt["grains.get"]("conductor:group") in ["jkp_back-testing"] %}
{% set ec_sec_name = "sec-01d5tz30vptmkdp0v093y6gc4n" %}
{% elif salt["grains.get"]("conductor:group") in ["jkp_backoffice-testing"] %}
{% set ec_sec_name = "sec-01d5tz329b5tmaa3fmpw3vhm63" %}
{% elif grains["yandex-environment"] in ["testing", "stress", "development"] %}
{% set ec_sec_name = "sec-01d5tz2wagwxsycz3ce46mnymp" %}
{% elif salt["grains.get"]("conductor:group") in ["jkp_back-prestable"] %}
{% set ec_sec_name = "sec-01d5tz2p8nq2cp9ntty194f36z" %}
{% elif salt["grains.get"]("conductor:group") in ["jkp_back-stable"] %}
{% set ec_sec_name = "sec-01d5tz2qstcsymqzcjbgv7fmz8" %}
{% elif salt["grains.get"]("conductor:group") in ["jkp_backoffice"] %}
{% set ec_sec_name = "sec-01d5tz2sa1zzsgmh15fqpa62px" %}
{% elif grains["yandex-environment"] in ["production", "prestable"] %}
{% set ec_sec_name = "sec-01d5tz2mrhx8pb4387xax2hc39" %}
{% endif %}
    content: {{ salt.yav.get(ec_sec_name+'[item]') | json }}
