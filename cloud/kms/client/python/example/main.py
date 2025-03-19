#!/usr/bin/env python

import argparse
import logging
import time

from kmsclient import KmsRoundRobinClient, KmsRoundRobinClientOptions


def main():
    logging.basicConfig(level=logging.DEBUG)

    parser = argparse.ArgumentParser()
    parser.add_argument("--addrs", help="comma-separated list of endpoints to connect to",
                        required=True)
    parser.add_argument("--token", help="IAM token", required=True)
    parser.add_argument("--tls-server-name",
                        help="replace hostname in TLS validation (useful for directly connecting to backends)")
    parser.add_argument("--root-ca", help="path to root CA (useful for preprod, e.g. /etc/ssl/certs/YandexInternalRootCA.pem)")
    parser.add_argument("--insecure", help="disable TLS", action='store_true')
    parser.add_argument("--key-id", help="key ID to use for encryption/decryption", required=True)
    parser.add_argument("--requests", help="number of encrypt/decrypt requests to do per thread",
                        default=100, type=int)
    parser.add_argument("--retries", help="maximum number of retries per request",
                        default=0, type=int)
    parser.add_argument("--data-size", help="size of plaintext", default=32, type=int)
    parser.add_argument("--log-level", help="additional logging for the KMS client", default="info")
    args = parser.parse_args()

    options = KmsRoundRobinClientOptions()
    options.token_provider = lambda: args.token

    if args.retries > 0:
        options.retries = args.retries

    options.log = logging.getLogger("KMS")
    if args.log_level == "info":
        options.log.setLevel(logging.INFO)
    elif args.log_level == "debug":
        options.log.setLevel(logging.DEBUG)

    if args.root_ca:
        with open(args.root_ca, "rb") as f:
            options.root_ca_pem = f.read()
    options.tls_server_name = args.tls_server_name
    options.insecure = args.insecure

    addrs = args.addrs.split(",")
    num_errors = 0
    encrypt_times = []
    decrypt_times = []
    total_start_time = time.monotonic()
    with KmsRoundRobinClient(addrs, options) as client:
        aad = b"this-is-aad"
        r = 0
        while r < args.requests:
            if r % 1000 == 0:
                logging.info("Request %d", r)
            if r % 2 == 0:
                plaintext = b"\x12" * args.data_size
                try:
                    start_time = time.monotonic()
                    ciphertext = client.encrypt(args.key_id, aad, plaintext)
                    encrypt_times.append(time.monotonic() - start_time)
                except Exception as e:
                    logging.critical("Got exception while encrypting: %s", e)
                    num_errors += 1
                    # Skip the next decrypt
                    r += 1
            else:
                try:
                    start_time = time.monotonic()
                    new_plaintext = client.decrypt(args.key_id, aad, ciphertext)
                    decrypt_times.append(time.monotonic() - start_time)
                except Exception as e:
                    logging.critical("Got exception while decrypting: %s", e)
                    num_errors += 1
                if new_plaintext != plaintext:
                    logging.critical("Different plaintexts: %s vs %s", plaintext, new_plaintext)
                    return
            r += 1
    total_time = time.monotonic() - total_start_time
    rps = int(args.requests / total_time)

    if len(encrypt_times) == 0 and len(decrypt_times) == 0:
        logging.critical("No encrypt or decrypt times")
        return

    encrypt_times.sort()
    s = len(encrypt_times)
    encrypt_p50 = encrypt_times[int(s / 2)]
    encrypt_p95 = encrypt_times[int(s * 95 / 100)]
    encrypt_p99 = encrypt_times[int(s * 99 / 100)]
    encrypt_p100 = encrypt_times[int(s - 1)]
    decrypt_times.sort()
    s = len(decrypt_times)
    decrypt_p50 = decrypt_times[int(s / 2)]
    decrypt_p95 = decrypt_times[int(s * 95 / 100)]
    decrypt_p99 = decrypt_times[int(s * 99 / 100)]
    decrypt_p100 = decrypt_times[int(s - 1)]

    logging.info("Decrypt p50: %dusec, p95: %dusec, p99: %dusec, p100: %dusec,"
                 " encrypt p50: %dusec, p95: %dusec, p99: %dusec, p100: %dusec",
                 int(decrypt_p50 * 1000000), int(decrypt_p95 * 1000000), int(decrypt_p99 * 1000000), int(decrypt_p100 * 1000000),
                 int(encrypt_p50 * 1000000), int(encrypt_p95 * 1000000), int(encrypt_p99 * 1000000), int(encrypt_p100 * 1000000))
    logging.info("Total RPS: %d, total errors: %d", rps, num_errors)

if __name__ == "__main__":
    main()
