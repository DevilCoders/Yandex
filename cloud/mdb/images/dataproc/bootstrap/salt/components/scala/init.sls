{% if salt['ydputils.is_presetup']() %}
scala-packages:
    pkg.installed:
        - sources:
            - scala: https://downloads.lightbend.com/scala/2.12.10/scala-2.12.10.deb
{% endif %}
