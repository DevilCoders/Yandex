import os

def main():
    user_shell = os.popen("echo $SHELL").read()
    if '/bin/zsh' in user_shell:
        file = '~/.zshrc'
    else:
        file = '~/.bashrc'
    cli_alias = os.popen(f"cat {file} | grep yc-cli").read()
    for x in cli_alias.split('\n'):
        if x != '':
            print(f"I found alias: {x[6:]}")
    alias = input("Enter your alias for cli: ")
    print("----------\n[Step-1] Enter client account data\n----------")
    client_billing_id = input("Enter BILLING_ID from which we transfer cloud: ")
    client_cloud_id = input("Enter CLOUD_ID that you want to transfer: ")
    client_account = input("Enter client account login: ")
    client_account = [x for x in os.popen(f"{alias} user-account show {client_account} -f value").read().split('\n')][0]
    print("----------\n[Step-2] Enter partner account data\n----------")
    partner_billing_id = input("Enter BILLING_ID where you want to transfer cloud: ")
    partner_cloud_id = input("Enter CLOUD_ID of empty [blank] cloud: ")
    partner_account = input("Enter partner account login: ")
    partner_account = [x for x in os.popen(f"{alias} user-account show {partner_account} -f value").read().split('\n')][0]
    os.system("clear")
    print("[Step-3] Check input")
    print(f"----------\n[Client]:\nb_id: {client_billing_id}\ncloud_id: {client_cloud_id}\naccount_id: {client_account}")
    print(f"----------\n[Partner]:\nb_id: {partner_billing_id}\ncloud_id: {partner_cloud_id}\naccount_id: {partner_account}\n----------")
    check = input("Continue? [y/n]: ")
    if check not in ["y","","д"," "]:
        print("Abort mission")
        return
    os.system("clear")
    print(f"----------\n[Step-4] Executing\n----------")
    print("[1] Binding client cloud to partner billing")
    os.system(f"{alias} cloud bind --billing_account {partner_billing_id} {client_cloud_id}")
    print("[2] Binding client billing to partner blank cloud")
    os.system(f"{alias} cloud bind --billing_account {client_billing_id} {partner_cloud_id}")
    print("[3] Granting client admin access to blank cloud")
    os.system(f"{alias} access-binding update --cloud-id {partner_cloud_id} --binding-delta action=add,accessBinding.roleId=resource-manager.clouds.member,accessBinding.subject.id={client_account},accessBinding.subject.type=userAccount")
    os.system(f"{alias} access-binding update --cloud-id {partner_cloud_id} --binding-delta action=add,accessBinding.roleId=admin,accessBinding.subject.id={client_account},accessBinding.subject.type=userAccount")
    print("[4] Granting partner member and admin access to moved cloud")
    os.system(f"{alias} access-binding update --cloud-id {client_cloud_id} --binding-delta action=add,accessBinding.roleId=resource-manager.clouds.member,accessBinding.subject.id={partner_account},accessBinding.subject.type=userAccount")
    os.system(f"{alias} access-binding update --cloud-id {client_cloud_id} --binding-delta action=add,accessBinding.roleId=admin,accessBinding.subject.id={partner_account},accessBinding.subject.type=userAccount")

if __name__ == "__main__":
    main()
