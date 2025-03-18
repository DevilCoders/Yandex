# import datetime
import logging
import yp.client
import dns2


def main():
    logging.basicConfig(level=logging.DEBUG)

    xdc_clt = yp.client.YpClient('xdc.yp.yandex.net:8090', config={'token': os.environ['YP_TOKEN']})
    forward, reverse = dns2.read(xdc_clt)
    data = [line.split() for line in open('./gencfg/_trunk.txt').readlines()]
    gencfg_list = [(line[0], line[1]) for line in data]
    update_aaaa, create_aaaa, update_ptr, create_ptr = dns2.calc_ypdns_update(forward, reverse, gencfg_list)

    # print update_aaaa, create_aaaa, create_ptr
    dns2.create_objs(xdc_clt, 'aaaa', create_aaaa, 100)
    dns2.create_objs(xdc_clt, 'ptr', create_ptr, 100)
    dns2.update_objs(xdc_clt, 'aaaa', update_aaaa, 100)
