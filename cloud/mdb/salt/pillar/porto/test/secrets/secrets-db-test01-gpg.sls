pubring.gpg: {{ salt.yav.get('ver-01eca240ka5majxecewa3wwksw[pubring.gpg]') | json }}
secring.gpg: {{ salt.yav.get('ver-01eca240ka5majxecewa3wwksw[secring.gpg]') | json }}
trustdb.gpg: {{ salt.yav.get('ver-01eca240ka5majxecewa3wwksw[trustdb.gpg]') | json }}
