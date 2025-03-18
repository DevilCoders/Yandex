import keyring # pip install keyring
import getpass

namespace = 'TOLOKA'
token_name = input('Set key-name for:')
token_val1 = getpass.getpass(prompt=f'Enter secret value for key "{token_name}":')
token_val2 = getpass.getpass(prompt='Confirm secret value:')

if token_val1 == token_val2:
    keyring.set_password(namespace, token_name, token_val1)
else:
    print('\nThe entered values do not match.')
    exit()

# retrieve password
check_val = keyring.get_password(namespace, token_name)
print(keyring.get_password(namespace, token_name))
print('\nSecret created:', check_val==token_val1)
