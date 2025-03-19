"""
OpenSSL-based PKI
"""
import os

from OpenSSL import crypto


class PKI:
    """
    OpenSSL-based PKI
    """

    def __init__(self, basedir='staging/images/fake_certificator/config/pki'):
        self.keys = {}
        self.certs = {}
        self.next_serial = 1
        self.basedir = basedir
        self.crl = None

        self.load()

        if not self.certs:
            self.gen_ca()

    def load(self):
        """
        Get keys and certs from basedir
        """
        if not os.path.exists(self.basedir):
            return
        for file_name in os.listdir(self.basedir):
            path = os.path.join(self.basedir, file_name)
            name, ext = os.path.splitext(file_name)
            with open(path, 'rb') as inp:
                if ext == '.key':
                    self.keys[name] = crypto.load_privatekey(crypto.FILETYPE_PEM, inp.read())
                elif ext == '.pem':
                    self.certs[name] = crypto.load_certificate(crypto.FILETYPE_PEM, inp.read())
                    self.next_serial = max(self.next_serial, self.certs[name].get_serial_number())
                elif name == 'crl':
                    self.crl = crypto.load_crl(crypto.FILETYPE_PEM, inp.read())
        if self.certs:
            self.next_serial += 1

    def dump(self):
        """
        Save keys and certs to basedir
        """
        os.makedirs(self.basedir, exist_ok=True)
        for name, key in self.keys.items():
            with open(os.path.join(self.basedir, '{name}.key'.format(name=name)), 'wb') as out:
                out.write(crypto.dump_privatekey(crypto.FILETYPE_PEM, key))
        for name, cert in self.certs.items():
            with open(os.path.join(self.basedir, '{name}.pem'.format(name=name)), 'wb') as out:
                out.write(crypto.dump_certificate(crypto.FILETYPE_PEM, cert))

        with open(os.path.join(self.basedir, 'crl'), 'wb') as out:
            out.write(self.crl.export(self.certs['CA'], self.keys['CA'], days=3560, digest=b'sha512'))

    def revoke(self, name):
        """
        Revoke cert
        """
        if name == 'CA':
            raise RuntimeError('Unable to revoke CA')

        if name not in self.certs:
            raise RuntimeError('Unable to revoke not issued cert')

        revoked = crypto.Revoked()
        revoked.set_serial(hex(self.certs[name].get_serial_number())[2:].encode('utf-8'))
        revoked.set_reason(b'Unspecified')
        revoked.set_rev_date(self.certs[name].get_notBefore())
        self.crl.add_revoked(revoked)

    def _gen_cert(self, name, key_len=2048):
        """
        Generate and sign cert
        """
        key = self._gen_key(name, key_len)
        cert = crypto.X509()
        self.certs[name] = cert
        self._set_common_opts(name, key)
        cert.add_extensions([
            crypto.X509Extension(b'basicConstraints', True, b'CA:FALSE'),
            crypto.X509Extension(b'keyUsage', True, b'digitalSignature, keyEncipherment'),
            crypto.X509Extension(b'extendedKeyUsage', True, b'TLS Web Server Authentication'),
            crypto.X509Extension(b'subjectAltName', False, 'DNS:{name}'.format(name=name).encode('utf-8')),
        ])
        cert.sign(self.keys['CA'], 'sha512')

    def _gen_key(self, name, key_len=2048):
        """
        Generate new key and save into keys
        """
        key = crypto.PKey()
        key.generate_key(crypto.TYPE_RSA, key_len)
        self.keys[name] = key
        return key

    def _set_common_opts(self, name, key):
        """
        Set common cert options (same for CA and certs)
        """
        cert = self.certs[name]
        cert.set_version(2)
        cert.set_serial_number(self.next_serial)
        self.next_serial += 1
        cert.get_subject().CN = name
        cert.gmtime_adj_notBefore(0)
        cert.gmtime_adj_notAfter(24 * 3600 * 3650)
        cert.set_issuer(self.certs['CA'].get_subject())
        cert.set_pubkey(key)

    def gen_ca(self):
        """
        Gen CA and inital crl
        """
        key = self._gen_key('CA', key_len=4096)

        cert = crypto.X509()
        self.certs['CA'] = cert
        self._set_common_opts('CA', key)
        cert.add_extensions([
            crypto.X509Extension(b'basicConstraints', True, b'CA:TRUE, pathlen:0'),
            crypto.X509Extension(b'keyUsage', True, b'keyCertSign, cRLSign'),
            crypto.X509Extension(b'subjectKeyIdentifier', False, b'hash', subject=cert),
        ])
        cert.sign(key, 'sha512')

        self.crl = crypto.CRL()

    def get_cert(self, name):
        """
        Return cert in PEM format
        """
        if name not in self.certs:
            self._gen_cert(name)
        return crypto.dump_certificate(crypto.FILETYPE_PEM, self.certs[name]).decode('utf-8')

    def get_key(self, name):
        """
        Return key in PEM format
        """
        if name not in self.certs:
            self._gen_cert(name)
        return crypto.dump_privatekey(crypto.FILETYPE_PEM, self.keys[name]).decode('utf-8')

    def get_crl(self):
        """
        Return crl
        """
        return self.crl.export(self.certs['CA'], self.keys['CA'], days=3560, digest=b'sha512').decode('utf-8')
