import re
import subprocess

CMD='/usr/bin/timeout -s KILL 3 /sbin/ethtool -l eth0'

def process(interfaces, params):
    for i in interfaces:
        if i.name == "ip6tun0":
            i.postup.append('/sbin/ifconfig ip6tnl0 up')
            i.postup.append('/bin/bash -c \'for i in $(ls -1 /proc/sys/net/ipv4/conf/*/rp_filter | grep -v L3); do echo 0 > $i; done\' || true')
            numtxqueues_num = "8"
            try:
                p = subprocess.Popen(CMD, shell=True, executable='/bin/bash', stdout=subprocess.PIPE)
                out, err = p.communicate()
                lines = out.split('\n')
                for line in lines:
                    if 'TX' in line or 'Combined' in line:
                        cnum = line.split()[-1]
                        if int(cnum) > 0 and 100 > int(cnum):
                            numtxqueues_num = cnum
            except:
                pass
            numtxqueues = "numtxqueues {0}".format(numtxqueues_num)
            ip_parm = "ip link add dev ip6tun0 mtu 1450 {0} type ip6tnl ".format(numtxqueues)
            new_preup = [re.sub(r"^.* remote ", "{0} remote ".format(ip_parm), s) if ' remote ' in s else None for s in i.preup]
            # Example output:
            # ip link add dev ip6tun0 mtu 1450 numtxqueues 61 type ip6tnl remote 2a02:6b8:0:3400::aaaa local 2a02:6b8:c0e:786:0:41af:84ac:80ff
            # Old formate like
            # ip -6 tunnel add ip6tun0 mode ipip6 numtxqueues 8 remote 2a02:6b8:b010:a0ff::1 local 2a02:6b8:c02:5d1:0:4345:776b:93ed
            # not valid
            i.preup = new_preup

def filter(params):
  return True

