import sys
import os
import subprocess

# pip install yandex-passport-vault-client -i https://pypi.yandex-team.ru/simple
from vault_client.instances import Production as VaultClient

"""
Updates acl yav token

Usage: update-acl-secret.py <ticket>
"""

vault_api_token = os.environ['YAV_TOKEN']  # OAuth token for vault
acl_secret_id = "sec-01cz38q7s2azaeqc4jrcstg5v6"


def update_acl_secret(ticket):
    with open("acl.json", "r") as f:
        acl_data = "".join(f.readlines())

        log = subprocess.getoutput("arc log acl.json | head -n 10 | grep \"revision:\"")
        cur_revision = int(log.split(" ")[1])

        client = VaultClient(
            authorization="OAuth {}".format(vault_api_token),
            decode_files=True,
        )

        secret = client.get_version(acl_secret_id)

        if secret["value"]["DNS-ACL"] == acl_data:
            print("No diff between acl on disk and in yav")
            return

        prev_revision = int(secret["comment"][-7:])
        comment = """Ticket: {}
Diff: https://a.yandex-team.ru/arc/diff/trunk/arcadia/cloud/security/dns-acl/acl.json?prevRev=r{}&rev=r{}
Rev: https://a.yandex-team.ru/arc/trunk/arcadia/cloud/security/dns-acl/acl.json?rev=r{}""".format(
            ticket, prev_revision, cur_revision, cur_revision
        )
        print(comment)
        print()
        cnt = input("Type yes to continue: ")
        if cnt == "yes":
            client.create_diff_version(
                secret["version"],
                [{"key": "DNS-ACL"}, {"key": "DNS-ACL", "value": acl_data}],
                comment=comment,
                check_head=True
            )
            print("Done")


def main():
    try:
        ticket = sys.argv[1]
        update_acl_secret(ticket)
    except IndexError:
        print("Usage: update-acl-secret.py <ticket>")



if __name__ == '__main__':
    main()
