{% from "components/greenplum/map.jinja" import gpdbvars with context %}

/etc/odyssey/ssl:
    file.directory:
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - makedirs: True
        - mode: 750
{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
        - require_in:
            - service: odyssey
{% endif %}

/etc/odyssey/ssl/server.key:
    file.managed:
        - contents_pillar: cert.key
        - template: jinja
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 600
        - require:
            - file: /etc/odyssey/ssl
{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
        - require_in:
            - service: odyssey
{% endif %}

/etc/odyssey/ssl/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - template: jinja
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 600
        - require:
            - file: /etc/odyssey/ssl
{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
        - require_in:
            - service: odyssey
{% endif %}

/etc/odyssey/ssl/allCAs.pem:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 600
        - require:
            - file: /etc/odyssey/ssl
{% if salt.pillar.get('data:dbaas:subcluster_name') == 'master_subcluster' %}
        - require_in:
            - service: odyssey
{% endif %}

/etc/greenplum/ssl:
    file.directory:
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - makedirs: True
        - mode: 750

/etc/greenplum/ssl/server.key:
    file.managed:
        - contents_pillar: cert.key
        - template: jinja
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 600
        - require:
            - file: /etc/greenplum/ssl
        - require_in:
            - service: greenplum-service

/etc/greenplum/ssl/server.crt:
    file.managed:
        - contents_pillar: cert.crt
        - template: jinja
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 600
        - require:
            - file: /etc/greenplum/ssl
        - require_in:
            - service: greenplum-service


/etc/greenplum/ssl/allCAs.pem:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 600
        - require:
            - file: /etc/greenplum/ssl
        - require_in:
            - service: greenplum-service

/home/{{ gpdbvars.gpadmin }}/.postgresql:
    file.directory:
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - makedirs: True
        - mode: 755

/home/{{ gpdbvars.gpadmin }}/.postgresql/root.crt:
    file.managed:
        - contents_pillar: cert.ca
        - template: jinja
        - user: {{ gpdbvars.gpadmin }}
        - group: {{ gpdbvars.gpadmin }}
        - mode: 600
        - require:
            - file: /home/{{ gpdbvars.gpadmin }}/.postgresql
        - require_in:
            - service: greenplum-service
