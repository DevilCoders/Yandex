"""
Install Yandex CA to certify bundle.
Fixes the 'certificate verify failed: self signed certificate in certificate chain' error
https://wiki.yandex-team.ru/security/ssl/sslclientfix/#vpython
"""

import certifi
import requests
# source: https://incognitjoe.github.io/adding-certs-to-requests.html
if __name__ == '__main__':
  cafile = certifi.where()

  with open('YandexInternalRootCA.pem', 'rb') as infile:
    customca = infile.read()

  with open(cafile, 'ab') as outfile:
    outfile.write(customca)

  print('That might have worked.')
