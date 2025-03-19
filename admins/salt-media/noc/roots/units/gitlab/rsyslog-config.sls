# https://st.yandex-team.ru/NOCDEV-7215#62908bbaec82601fc5744e31
{%- load_yaml as log_files %}
- sshd/current
- gitlab-workhorse/current

- gitaly/gitlab-shell.log
- gitlab-shell/gitlab-shell.log

- gitlab-rails/api_json.log
- gitlab-rails/application_json.log
- gitlab-rails/audit_json.log
- gitlab-rails/auth.log
- gitlab-rails/graphql_json.log
- gitlab-rails/production_json.log

- nginx/gitlab_access.log
- nginx/gitlab_registry_access.log
{%- endload %}

/etc/rsyslog.d/60-gitlab.conf:
  file.managed:
    - user: root
    - group: root
    - mode: 666
    - makedirs: True
    - contents: |
        module(load="imfile")

        ## input files
        {% for file in log_files -%}
        input (type="imfile" file="/data/gitlab/logs/{{file}}" tag="{{file}}" ruleset="fwd-gitlab")
        {% endfor -%}
        ## convert to json template
        template(name="fwd-json" type="list" option.jsonf="on") {
            property(outname="ts" name="timegenerated" dateFormat="rfc3339" format="jsonf")
            property(outname="host" name="hostname" format="jsonf")
            property(outname="log" name="syslogtag" format="jsonf")
            property(outname="msg" name="msg" format="jsonf")
        }
        ## output fwd
        ruleset(name="fwd-gitlab") {
          action(type="omfwd" target="localhost" port="10514" protocol="udp" template="fwd-json")
        }
        # Override defaults to be able to read the log
        $PrivDropToUser root
        $PrivDropToGroup root

rsyslog:
  cmd.run:
    - name: rsyslogd -N1 -f /etc/rsyslog.conf
    - onchanges:
      - file: /etc/rsyslog.*
  service.running:
    - enable: True
    - watch:
      - file: /etc/rsyslog.*
    - require:
      - cmd: rsyslog
