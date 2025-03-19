from ipaddress import ip_address, IPv4Address
import re

def valid_ip(ip: str) -> str:
    try:
        ip_address(ip)
    except ValueError:
        return False
    return True


def get_ip_from_text(text):
    if text:
        text = text.replace("\\", " ")
        text = text.lower()
        text = re.sub('\-\s\r\n\s{1,}|\-\s\r\n|\r\n', ' ', text)
        text = re.sub(
            '[,;_%©?*,!@#$%^&()]|[+=]|[«»]|[<>]|[\']|[[]|[]]|[/]|"|\s{2,}|-', ' ', text)
        text = ' '.join(word for word in text.split() if len(word) > 1)
        text = ' '.join(word for word in text.split() if not word.isnumeric())
        text = list(set([word for word in text.split() if valid_ip(word)]))
        return text