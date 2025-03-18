import argparse
import base64
import pprint
import sys

import jwt


def main():
    args = parse_args()

    if args.command == "parse-narwhal":
        return parse_narwhal(args)
    elif args.command == "parse-market":
        return parse_market(args)
    else:
        raise Exception("unknown command")


def parse_narwhal(args):
    narwhal_key = JwsKey.load_narwhal(args.narwhal_key)
    balancer_key = None

    if args.balancer_key is not None:
        balancer_key = JwsKey.load_narwhal(args.balancer_key)

    try:
        header = jwt.get_unverified_header(args.jws)
    except jwt.PyJWTError as exc:
        eprint(f"Failed to decode header: {exc}")
        return 1

    key_id = None

    eprint("Header:", end=" ")
    pprint.pprint(header, stream=sys.stderr)

    try:
        claims = jwt.decode(args.jws, options={"verify_signature": False})
    except jwt.PyJWTError as exc:
        eprint(f"Failed to decode claims: {exc}")
        return 1

    claims_state = "unverified"

    try:
        if "kid" in header:
            key_id = header["kid"]
            eprint(f"Key id: {key_id}")

            if key_id == narwhal_key.kid:
                narwhal_key.decode(args.jws)
                claims_state = "verified, narwhal"
            elif key_id == balancer_key.kid:
                balancer_key.decode(args.jws)
                claims_state = "verified, balancer"
            else:
                eprint("Unrecognized key id, cannot verify.")
        else:
            eprint("Key id unspecified, assuming narwhal.")

            narwhal_key.decode(args.jws)
            claims_state = "verified, narwhal"
    except jwt.PyJWTError as exc:
        eprint(f"Failed to verify: {exc}")

    eprint(f"Claims ({claims_state}):", end=" ")
    pprint.pprint(claims, stream=sys.stderr)


def parse_market(args):
    key = JwsKey.load_market(args.market_key)

    try:
        header = jwt.get_unverified_header(args.jws)
    except jwt.PyJWTError as exc:
        eprint(f"Failed to decode header: {exc}")
        return 1

    eprint("Header:", end=" ")
    pprint.pprint(header, stream=sys.stderr)

    try:
        claims = jwt.decode(args.jws, options={"verify_signature": False})
    except jwt.PyJWTError as exc:
        eprint(f"Failed to decode claims: {exc}")
        return 1

    claims_state = "unverified"

    try:
        key.decode(args.jws)
        claims_state = "verified, market"
    except jwt.PyJWTError as exc:
        eprint(f"Failed to verify: {exc}")

    eprint(f"Claims ({claims_state}):", end=" ")
    pprint.pprint(claims, stream=sys.stderr)


def parse_args():
    parser = argparse.ArgumentParser(
        description="parse narwhal or market JWS token",
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )

    subparsers = parser.add_subparsers(dest="command")
    add_parse_narwhal_cmd(subparsers)
    add_parse_market_cmd(subparsers)

    return parser.parse_args()


def add_parse_narwhal_cmd(subparsers):
    parser = subparsers.add_parser("parse-narwhal", help="parse narwhal JWS token (X-Yandex-Jws)")

    parser.add_argument(
        "--narwhal-key",
        required=True,
        help="path to narwhal key file",
    )

    parser.add_argument(
        "--balancer-key",
        default=None,
        help="path to balancer key file",
    )

    parser.add_argument(
        "--jws",
        required=True,
        help="JWS token to parse",
    )


def add_parse_market_cmd(subparsers):
    parser = subparsers.add_parser("parse-market", help="parse market JWS token (X-Jws)")

    parser.add_argument(
        "--market-key",
        required=True,
        help="path to market key file",
    )

    parser.add_argument(
        "--jws",
        required=True,
        help="JWS token parse",
    )


class JwsKey:
    def __init__(self, kid, algorithm, content):
        self.kid = kid
        self.algorithm = algorithm
        self.content = content

    @classmethod
    def load_narwhal(cls, path):
        with open(path) as file:
            file_content = file.read()

        tokens = file_content.strip().split(":")

        if len(tokens) != 3:
            raise Exception(f"{path}: invalid JWS key format, expected KEY_ID:ALGORITHM:KEY")

        try:
            key_content = bytes.fromhex(tokens[2])
        except Exception:
            raise Exception(f"{path}: invalid JWS key format, key must be hex-encoded")

        return cls(tokens[0], tokens[1], key_content)

    @classmethod
    def load_market(cls, path):
        with open(path) as file:
            file_content = file.read()

        try:
            key_content = base64.b64decode(file_content.strip())
        except Exception:
            raise Exception(f"{path}: invalid JWS key format: {exc}")

        return cls(None, "HS256", key_content)

    def decode(self, token):
        return jwt.decode(token, key=self.content, algorithms=[self.algorithm])


def eprint(*args, **kwargs):
    return print(*args, **kwargs, file=sys.stderr)


if __name__ == "__main__":
    sys.exit(main() or 0)
