import socket

def run():

    ips = set()

    for ip in set([i[4][0] for i in socket.getaddrinfo(socket.gethostname(), None)]):
        ips.add(ip)

    return ips
