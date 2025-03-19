/etc/salt/pki/master/master.pem:
   file.managed:
     - contents_pillar: {{ 'salt:secret_key' }}
     - mode: 0600

/etc/salt/pki/master/master.pub:
   file.managed:
     - contents_pillar: {{ 'salt:public_key' }}
     - mode: 0600
