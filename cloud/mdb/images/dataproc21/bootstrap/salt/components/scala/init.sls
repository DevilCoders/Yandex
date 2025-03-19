{% if salt['ydputils.is_presetup']() %}
scala-packages:
    pkg.installed:
        - sources:
            - scala: https://downloads.lightbend.com/scala/2.12.15/scala-2.12.15.deb
{% endif %}
