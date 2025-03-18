#!/usr/bin/env python

import sys

# Should be add a path where rsa and pyasn1 modules are placed
sys.path.append('/home/dude/contrib')

import rsa
import base64
import struct
from string import  maketrans

TRANS_TABLE = maketrans('-_', '+/')

keyText = '''
-----BEGIN RSA PUBLIC KEY-----
MGgCYQDonNLZo0J7t0SZtY4eDl8Y6WI0uLB2zr6GCyqTmdsfD4xXxKpeQqc6dyFm
WN78B5PO4pKtmuCyl9hcqIo+TsSARGw57FHFCaQNXsLsb+504DOWwEPnQh91ahUa
OLctEScCAwEAAQ==
-----END RSA PUBLIC KEY-----
'''

rsaKey = rsa.PublicKey.load_pkcs1(keyText)

def IsFuidValid(fuid, rsaKey):
    fs = fuid.strip().split('.')
    time, rand =int(fs[0][0:8], 16), int(fs[0][8:16], 16)
    sign = base64.decodestring('%s==' % fs[1].translate(TRANS_TABLE))

    data = struct.pack('!LL', time, rand)
    try:
        rsa.verify(data, sign, rsaKey)
        return True

    except:
        return False


if __name__ == '__main__':
    for i in sys.stdin:
        i = i.strip()
        print "%s\t%d" % (i, int(IsFuidValid(i, rsaKey)))
#    CheckFuid(i, rsaKey)
