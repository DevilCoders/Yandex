#sed -i "s/\(^.*ip6-loopback\).*/\1 localhost/" /etc/hosts

resolve "localhost" name into "::1":
  file.replace:
    name: /etc/hosts
    pattern: '::1 ip6-localhost ip6-loopback'
    repl: '::1 ip6-localhost ip6-loopback localhost'


remove-supervisord-confd:
   file.directory:
      - name: /etc/supervisord/conf.d/
      - clean: True
