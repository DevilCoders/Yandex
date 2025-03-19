import argparse
import base64


import pgpy
from pgpy.constants import CompressionAlgorithm, HashAlgorithm, KeyFlags, PubKeyAlgorithm, SymmetricKeyAlgorithm
from cloud.mdb.scripts.pillar_generators.lockbox import LockBox, TextValue, BinaryValue


def _generate(name, email):
    key = pgpy.PGPKey.new(PubKeyAlgorithm.RSAEncryptOrSign, 4096)
    uid = pgpy.PGPUID.new(name, comment=name, email=email)

    key.add_uid(
        uid,
        usage={
            KeyFlags.Sign,
            KeyFlags.EncryptCommunications,
            KeyFlags.EncryptStorage,
        },
        hashes=[
            HashAlgorithm.SHA256,
            HashAlgorithm.SHA384,
            HashAlgorithm.SHA512,
            HashAlgorithm.SHA224,
        ],
        ciphers=[
            SymmetricKeyAlgorithm.AES256,
            SymmetricKeyAlgorithm.AES192,
            SymmetricKeyAlgorithm.AES128,
        ],
        compression=[
            CompressionAlgorithm.ZLIB,
            CompressionAlgorithm.BZ2,
            CompressionAlgorithm.ZIP,
            CompressionAlgorithm.Uncompressed,
        ],
    )

    return base64.b64encode(str(key).encode('utf-8')).decode('utf-8'), key.fingerprint.keyid


def main():
    parser = argparse.ArgumentParser(
        description="""Generate PGP key,
         store them in LockBox,
         grant service account permission to access that key"""
    )
    parser.add_argument(
        'name',
        help='cluster name',
    )
    parser.add_argument(
        '--suffix',
        default='gpg',
    )
    parser.add_argument(
        '--ycp-profile',
        required=True,
    )
    parser.add_argument(
        '--folder-id',
        required=True,
    )
    parser.add_argument('--service-account-id', help='grant that service account access to all found secrets')
    parser.add_argument('--email', default='mdb-admin@yandex-team.ru')

    args = parser.parse_args()

    lockbox = LockBox(args.ycp_profile, args.folder_id)
    res = _generate(args.name, email=args.email)

    secret_id = lockbox.create_secret(
        f'{args.name}.{args.suffix}', BinaryValue('gpg_key', res[0]), TextValue('gpg_key_id', res[1])
    )
    if args.service_account_id:
        lockbox.add_access_binding(secret_id, args.service_account_id)
    print(
        """
data:
    s3: {{ salt.lockbox.get("%s") | tojson }}
"""
        % secret_id
    )


if __name__ == '__main__':
    main()
