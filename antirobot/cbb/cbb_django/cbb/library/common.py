from socket import gethostbyaddr

from antirobot.cbb.cbb_django.cbb.library.errors import InvalidRangeError
from django.shortcuts import render_to_response
from django.utils.translation import ugettext_lazy as _
from ipaddr import IPAddress, IPNetwork

RANGE = "0"
SINGLE_IP = "1"
CIDR = "2"
TXT = "3"
RE = "4"

ranges_types = {
    RANGE: "range",
    SINGLE_IP: "single_ip",
    CIDR: "cidr",
    TXT: "txt",
    RE: "re",
}
types_ranges = dict([(v, k) for k, v in ranges_types.items()])


# data preparation connected function
def get_range_from_net(net_ip, net_numhosts, version=4, result="bin"):
    assert version in {4, 6}
    # TODO: Error hadling for ippadr?
    net = IPNetwork(net_ip + f"/{net_numhosts}", version)
    if result == "ip":
        return net
    elif version == 4 or (version == 6 and result == "int"):
        return (int(net.network), int(net.broadcast))
    elif version == 6 and result == "bin":
        return (net.network.packed, net.broadcast.packed)
    else:
        return


def get_ip_from_string(ip, version=4, result="bin"):
    """
    ip в виде строки превращает в форму, удобную для поиска по базе
    ip - число или строка в формате ip
    """
    assert version in (4, 6)
    # TODO: Error hadling for ippadr?
    # TODO: make good signature
    ip = IPAddress(ip, version)
    if version == 4 or (version == 6 and result == "int"):
        return int(ip)
    elif version == 6 and result == "bin":
        return ip.packed
    else:
        return


def compare_ips(first, second, check_length=False):
    first = IPAddress(first)
    second = IPAddress(second)
    if first.version != second.version:
        raise InvalidRangeError(_("Different ip versions"))
    if first > second:
        raise InvalidRangeError(_("Range\"s start is greater than end"))
    if check_length and int(second) - int(first) > 255:
        raise InvalidRangeError(_("Range is unacceptable large"))
    return first.version


def get_user(request):
    if request.yauser and request.yauser.is_authenticated():
        return request.yauser.login
    ip = request.META.get("X-Real-Ip", request.META.get("REMOTE_ADDR", ""))
    try:
        return gethostbyaddr(ip)[0]
    except:
        return ip


def get_full_range(rng_start, rng_end):
    if rng_start == rng_end:
        return rng_start
    else:
        return rng_start + " " + rng_end


def response_forbidden():
    response = render_to_response("cbb/403.html")
    response.status_code = 403
    return response


def get_bin_id(block):
    """
    Get bin_id from regex
    """
    block_txt = block.rng_txt.split(';')
    exp_bin_matches = [i for i in range(len(block_txt)) if block_txt[i].startswith("exp_bin=")]
    if (len(exp_bin_matches) > 1):
        res_bin = None
    elif (len(exp_bin_matches) == 1):
        res_bin = block_txt[exp_bin_matches[0]][len("exp_bin="):]
    else:
        res_bin = None
    return res_bin


def change_bin(rng_txt, exp_bin):
    """
    exp_bin = -2 : do nothing
            = -1 : delete exp_bin from rng_txt if any
            in [0, 1, 2, 3] change existed exp_bin or add new
    """

    if exp_bin is not None and exp_bin != '-2':
        block_regex = rng_txt.split(';')
        block_regex = [part for part in block_regex if len(part) > 0]

        exp_bin_matches = [i for i in range(len(block_regex)) if block_regex[i].startswith("exp_bin=")]
        if (len(exp_bin_matches) > 1):
            exp_bin_matches = [exp_bin_matches[-1]]
        if (len(exp_bin_matches) == 1):
            if exp_bin != '-1':
                block_regex[exp_bin_matches[0]] = f"exp_bin={exp_bin}"
            else:
                del block_regex[exp_bin_matches[0]]
        else:
            if exp_bin != '-1':
                block_regex.append(f"exp_bin={exp_bin}")
        block_regex = ";".join(block_regex)

        return block_regex
    return rng_txt
