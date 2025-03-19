vncserver:
  users:
    - name: aarkhincheev
      password: {{ salt.yav.get("sec-01dvzjc3xthe2xap9bhrs2p6ka[password]")|json }}
    - name: nataliart
      password: {{ salt.yav.get("sec-01dvzjphytf3ytwr103pdq7502[password]")|json }}
    - name: ankozyrev
      password: {{ salt.yav.get("sec-01dvzjqs3zjhax5nnsrten0stf[password]")|json }}
    - name: iliyakutuzov
      password: {{ salt.yav.get("sec-01dvzjv91way9yhf324nh3baaj[password]")|json }}
    - name: poludasha
      password: {{ salt.yav.get("sec-01dt9gbqecca797ddy7dde40v4[password]")|json }}
    - name: antonmalashin
      password: {{ salt.yav.get("sec-01dt9gexdk0487efpwtym6geak[password]")|json }}
    - name: ivanov-mdlab
      password: {{ salt.yav.get("sec-01eg3gd3s0a395hr28jjafxjsh[password]")|json }}
    - name: aharitonova
      password: {{ salt.yav.get("sec-01dvzjxx3hn0pt7wqh334gfxcy[password]")|json }}
    - name: masavelyeva
      password: {{ salt.yav.get("sec-01dvzjjtsr9685rs71djwnzqgb[password]")|json }}
    - name: anna-sokolins
      password: {{ salt.yav.get("sec-01ejbayy61efda8dbcb49bqqv0[password]")|json }}
    - name: gneser
      password: {{ salt.yav.get("sec-01f4a0z5987bxyvwsd4zd91h1w[password]")|json }}
    - name: videoslava
      password: {{ salt.yav.get("sec-01fxszw37yzm535g22yk6gzbpv[password]")|json }}

  users_string: "2:aarkhincheev 4:masavelyeva 5:nataliart 6:ankozyrev 8:iliyakutuzov 10:aharitonova 11:poludasha 12:antonmalashin 13:ivanov-mdlab 14:anna-sokolins 15:gneser 16:videoslava"
