#!/usr/bin/env python
# -*-coding: utf-8 -*-
# vim: sw=4 ts=4 expandtab ai

import os
import re
import signal
import socket
import sys
import threading
import time
import pickle
import struct
from select import select

from log import grcl_logger
from misc import config_parser, log_init, get_ip, write_monitoring, write_uniq


# Default path to config
_PATH_TO_CONFIG = '/etc/yabs-graphite-client/graphite-client.cfg'
# Regexp for check input metric
_METRIC_RE = re.compile(r'^[-a-zA-Z0-9_]+\.[-a-zA-Z0-9_]+\.\S+\s+[-0-9.eE+]+\s+\d{8,10}$')
# Regexp for parsing servernames
_SERVER_RE = re.compile(r'([^:]+)\:(\d+)(?:\:(.+))?')

# start changes from cocaine for QLOUD env
_QLOUDREPLECEFQDN_RE = re.compile(r'i[-_][0-9a-zA-Z]{12}[\._]qloud[-_]c[\._]yandex[\._]net')
# end

# flag for all threads to exit from main cycles
_exit_flag = threading.Event()
signal.signal(signal.SIGTERM, lambda *a: stop_daemon())


def stop_daemon():
    log = grcl_logger.manager.getLogger('GRCL.STOP')
    log.info('Get SIGTERM - set exit flag')
    _exit_flag.set()

def main():
    config = config_parser(_PATH_TO_CONFIG)

    log_init(config['SENDER_LOG_FILE'], config['LOG_LEVEL'])
    log = grcl_logger.manager.getLogger('GRCL.Reciever')

    # shared list for results from clients
    results_list = []
    write_to_result_lock = threading.Lock()

    # create server socket
    bscr = []
    use_two_socks = False
    try:
        if config['SENDER_LISTEN_PR'] == socket.AF_INET + socket.AF_INET6:
            sock_types = [socket.AF_INET, socket.AF_INET6]
            use_two_socks = True
        else:
            sock_types = [config['SENDER_LISTEN_PR']]

        for sock_type in sock_types:
            try:
                srvsock = socket.socket(sock_type, socket.SOCK_STREAM)
                srvsock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                if use_two_socks and sock_type == socket.AF_INET6:
                    # Disable DualSocket on ipv6 socket
                    srvsock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, 1)
                srvsock.bind( (config['SENDER_LISTEN_HOST'], config['SENDER_PORT']) )
                srvsock.listen(65536)
                bscr.append(srvsock)
            except socket.gaierror as err:
                log.critical('Can\'t create socket: %s' % err.message)
            if not bscr: raise
    except Exception, err:
        log.critical('Can\'t create any socket: %s' % err.message)
        sys.exit(1)

    # Run thread for send data to graphite
    thr = threading.Thread(target=send_to_graphite_thread, args=(config, results_list, write_to_result_lock))
    thr.setDaemon(True)
    thr.start()

    # list for select in all sockets - client and server
    descr = []
    descr.extend(bscr)
    # dict for not parsed data from clients
    data_from_clients = {}
    log.info('Start receiver thread')
    while True:
        # Await an event on a readable socket descriptor
        try:
            sread, _swrite, _sexc = select(descr, [], [])
        except Exception, err:
            if _exit_flag.isSet():
                log.info('Exit from select on receiver thread')
                break
            else:
                log.critical('Error select from socket: %s' % err.message)
        # Iterate through the tagged read descriptors
        for sock in sread:
            # Received a connect to the server (listening) socket
            if sock in bscr:
                srvsock = bscr[bscr.index(sock)]
                newsock, rem_addr_info = srvsock.accept()
                remhost = rem_addr_info[0]
                remport = rem_addr_info[1]
                descr.append(newsock)
                new_sock_id = '%s_%d_%d' % (remhost, remport, newsock.fileno())
                data_from_clients[new_sock_id] = ''
                log.debug('New connection from %s %s' % \
                        (remhost, remport))
            # Received something on a client socket
            else:
                addr_info = sock.getpeername()
                sock_addr = addr_info[0]
                sock_port = addr_info[1]
                sock_id = '%s_%d_%d' % (sock_addr, sock_port, sock.fileno())
                # Check to see if the peer socket closed
                from_sock = sock.recv(2048)
                if from_sock == '':
                    # write to shared result list
                    if data_from_clients[sock_id]:
                        client_results = data_from_clients[sock_id].split('\n')
                        log.debug('Get %s metrics from client' % \
                                len(client_results))
                        write_to_result_lock.acquire()
                        try:
                            results_list += client_results
                        finally:
                            write_to_result_lock.release()
                    # rm all socket info
                    sock.close()
                    descr.remove(sock)
                    del data_from_clients[sock_id]
                else:
                    data_from_clients[sock_id] += from_sock
                    # Split data for long connections
                    if len(data_from_clients[sock_id]) >= 4096:
                        client_results = data_from_clients[sock_id].split('\n')
                        log.debug('Get %s metrics from client long conn' % \
                                (len(client_results)-1))
                        write_to_result_lock.acquire()
                        try:
                            results_list += client_results[:-1]
                        finally:
                            write_to_result_lock.release()
                        data_from_clients[sock_id] = client_results[-1]
        # exit by exit signal
        if _exit_flag.isSet():
            log.info('Exit from receiver thread')
            break


def send_to_graphite_thread(config, results_list, write_to_result_lock):

    log = grcl_logger.manager.getLogger('GRCL.Sender')
    send_log = grcl_logger.manager.getLogger('GRCL.SendDataWorker')

    # Struct for write info about send to mon file
    mon_stat = {
        'send_time': 0,
        'total_srv':len(config['SERVER_LIST']),
        'sucess_srv':0,
        'failed_srv': []
        }

    # Count of getted from receiver thread metric
    sended_metric_count = 0
    dropped_metric_count = 0
    prev_send_metric_cnt_time = int(time.time())
    delay_send_cache = []
    max_delay_send_lim = config['MAX_METRIC_COUNT'] * 10
    max_send_lim = config['MAX_METRIC_COUNT']

    start_send_time = time.time()
    log.info('Start sender thread')
    uniq_metrics = set()

    while True:
        # wait timeout for aggregate clients data
        if (time.time() - start_send_time) < config['SENDER_TIMEOUT']:
            time.sleep(abs(config['SENDER_TIMEOUT'] - (time.time() - start_send_time)))
        start_send_time = time.time()

        if not results_list:
            # if we need send delay cache
            if delay_send_cache:
                data_list = []
            else:
                continue
        else:
            # Get data from shared list:
            write_to_result_lock.acquire()
            try:
                data_list = results_list[:]
                # complete clear shared list
                del results_list[:]
            finally:
                write_to_result_lock.release()
        get_results_time = time.time() - start_send_time

        # Prepare clients data
        data_for_send = []

        # start changes from cocaine for QLOUD env
        qloud_hostname = os.getenv('PORTO_HOST', "nohostnameenv")
        newfqdnmarker = "ape-test-qloud-" + qloud_hostname
        config['HOSTNAME'] = newfqdnmarker
        newfqdnmarker = re.sub(r'[\.-]', "_", newfqdnmarker)
        # end


        for metric in data_list:
            metric = metric.strip()
            # skip empty lines
            if not metric:
                continue

            # check metric

            if _METRIC_RE.match(metric):
                # start changes from cocaine for QLOUD env
                metric = re.sub(_QLOUDREPLECEFQDN_RE, newfqdnmarker, metric)
                #end
                data_for_send.append(metric)
                metric_name = metric.split()[0]
                uniq_metrics.add(metric_name)

            else:
                log.warning('Incorrect metric: %s. Skip it.' % metric)
        prepare_metrics_time = time.time() - start_send_time

        # try send old metrics in delay cache
        data_for_send += delay_send_cache
        # Check send limit
        if sended_metric_count + len(data_for_send) <= max_send_lim:
            sended_metric_count += len(data_for_send)
            delay_send_cache = []
        else:
            send_limit = max_send_lim - sended_metric_count

            sended_metric_count += send_limit
            cur_dropped_metric = len(data_for_send[ (send_limit + max_delay_send_lim) : ])
            dropped_metric_count += cur_dropped_metric
            if cur_dropped_metric:
                log.error('Get too many metrics: %d. Drop it.' % \
                        cur_dropped_metric)
                if config['DROPPED_METRIC_FILE']:
                    log.info('Write dropped metrics to %s' % \
                            config['DROPPED_METRIC_FILE'])
                    drop_list = data_for_send[(send_limit + max_delay_send_lim):]
                    dropped = dict((m.split()[0], (m.split()[1], m.split()[-1])) for m in drop_list)
                    write_monitoring(config['DROPPED_METRIC_FILE'], dropped, log)

            delay_send_cache = data_for_send[send_limit : (send_limit+max_delay_send_lim)]
            data_for_send = data_for_send[:send_limit]

        # Add sended metric count to sended data every min
        cur_time = time.time()
        if cur_time - prev_send_metric_cnt_time >= 60.0:
            prev_send_metric_cnt_time = cur_time
            data_for_send.append('%s.%s.sended_metric %d %d' % \
                    (config['MONITORING_PREFIX'], config['HOSTNAME'], sended_metric_count, int(cur_time)))
            data_for_send.append('%s.%s.droped_metric %d %d' % \
                    (config['MONITORING_PREFIX'], config['HOSTNAME'], dropped_metric_count, int(cur_time)))
            data_for_send.append('%s.%s.cached_metric %d %d' % \
                    (config['MONITORING_PREFIX'], config['HOSTNAME'], len(delay_send_cache), int(cur_time)))
            data_for_send.append('%s.%s.uniq_metric %d %d' % \
                    (config['MONITORING_PREFIX'], config['HOSTNAME'], len(uniq_metrics), int(cur_time)))
            sended_metric_count = 0
            dropped_metric_count = 0

        send_to_server_time = []
        if data_for_send:
            # Send data
            for server in config['SERVER_LIST']:
                (resolve_time, connect_time, send_time) = \
                        send_check_data_worker((server, data_for_send[:], config, send_log, mon_stat))
                send_to_server_time.append((resolve_time, connect_time, send_time, (time.time()-start_send_time)) )
        else:
            log.warning('Not data for send. Only bad metrics or count over limit?')
            continue

        end_send_time = time.time() - start_send_time

        # Check timings
        if end_send_time >= 10.0:
            log.warning('Get result time: %.4f' % get_results_time)
            log.warning('Check data time: %.4f' % \
                    (prepare_metrics_time - get_results_time) )
            last_time = prepare_metrics_time
            for server, send_time in \
                    zip(config['SERVER_LIST'], send_to_server_time):
                log.warning('Resolve name for %s time: %.4f' % \
                        (server, send_time[0]))
                log.warning('Connect to %s time: %.4f' % \
                        (server, send_time[1]))
                log.warning('Send to %s time: %.4f' % \
                        (server, send_time[2]))
                log.warning('Total send for %s time: %.4f' % \
                        (server, send_time[3]-last_time))
                last_time = send_time[3]
            log.warning('Total send time: %.4f' % end_send_time)

        # Write to monitoring file
        mon_stat['send_time'] = float(end_send_time)
        write_monitoring(config['SENDER_MONITORING_FILE'], mon_stat, log)

        # Clear sucess send count
        mon_stat['sucess_srv'] = 0

        if len(uniq_metrics) > config['MAX_UNIQ_METRICS']:
            write_uniq(str(config['UNIQ_MONITORING_FILE']), uniq_metrics, log)
        if len(uniq_metrics) > 5*config['MAX_UNIQ_METRICS']:
            uniq_metrics.clear()
        # exit by exit signal
        if _exit_flag.isSet():
            log.info('Exit from sender thread')
            break


def send_check_data_worker(data):
    """
    Function worked in thread and send data to one graphite server
    """
    server, results_list, cfg, log, mon_stat = data

    data_format = 'plain'
    parsed = _SERVER_RE.match(server)
    if parsed is None:
        log.error('Can\'t parse port and hostname from %s' % server)
        port = None
        host = None
    else:
        host, port, data_format = parsed.groups()
        port = int(port)
        if data_format not in ('plain', 'pickle'):
            if data_format is not None:
                log.error('Invalid data format "%s" for host %s, falling back to "plain"' % \
                        (data_format, host))
            data_format = 'plain'

    # Path to old data
    data_file_name = os.path.join(cfg['QUEUE_DIR'], server.replace(':','_'))
    old_results_list = []
    # Check old not sended data for this server
    if os.path.exists(data_file_name) and os.path.isfile(data_file_name):
        nowtime = time.time()
        log.debug('Read old data from %s' % data_file_name)
        for line in open(data_file_name):
            timestamp = None
            try:
                timestamp = int(line.split()[-1])
            except ValueError:
                log.error('Error parse line in %s: %s' % \
                        (data_file_name, line))
            # Use only newest data
            if timestamp:
                if (nowtime - timestamp) < cfg['QUEUE_TIMEOUT']:
                    old_results_list.append(line.strip())
    # Adding metric prefix to data
    if cfg['GLOBAL_METRIC_PREFIX']:
        prefix = cfg['GLOBAL_METRIC_PREFIX']
        # Removing trailing dot
        prefix = prefix.rstrip('.')
        for index, item in enumerate(results_list):
            # Skipping prefix addition if metric already starts with prefix
            if not item.startswith(prefix):
                results_list[index] = "%s.%s" % ( prefix, item )

    sended_data = old_results_list + results_list
    sended_data.reverse()

    if data_format == 'pickle':
        # convert data to carbon pickle format
        payload = []
        for item in sended_data:
            try:
                metric, value, timestamp = item.strip().split()
                value = float(value)
                timestamp = int(float(timestamp))
                payload.append( (metric, (timestamp, value)) )
            except Exception, e:
                log.debug('Error converting data "%s" to pickle format: %s' % \
                        ( item, str(e)))
                continue
        pickle_data_length = len(payload)
        payload = pickle.dumps(payload)
        header = struct.pack("!L", len(payload))
        pickle_data = header + payload

    start_send_server_time = time.time()
    connect_time = 0
    send_time = 0
    if host:
        ip_proto_list, _fqdn = get_ip(host, log, cfg['SERVER_CHECK_PTR'])
    else:
        ip_proto_list = None
    resolve_time = time.time() - start_send_server_time

    # Try send to server
    if ip_proto_list:
        sucess = len(ip_proto_list)
        fail_msg = ''
        # Try connect to ipv4 or ipv6 addr
        for ip, proto in ip_proto_list:
            sock = None
            try:
                sock = socket.socket(proto, socket.SOCK_STREAM)
                sock.settimeout(2.0)
                sock.connect( (ip, port) )
                connect_time = time.time() - start_send_server_time - resolve_time
                break
            except Exception, err:
                fail_msg += 'server:%s, Err:%s;' % (server, err.message)
                sucess -= 1
                if sock:
                    sock.close()

        # Send data to connected socket
        if sucess and sock:
            try:
                log.debug('Send %d params to %s' % (len(sended_data), server))

                if data_format == 'plain':
                    send_count = 0
                    while sended_data:
                        # Send data and remove it data from list
                        sock.send(sended_data.pop() + '\n')
                        send_count += 1
                    send_time = time.time() - start_send_server_time - \
                            resolve_time - connect_time

                    log.debug('Sucess send %d params to %s' % \
                            (send_count, server))
                else:
                    sock.sendall(pickle_data)
                    sended_data = []
                    log.debug('Successfully sent %d pickled params to %s' % \
                            ( pickle_data_length, server ))

                # Write data to mon
                mon_stat['sucess_srv'] += 1

            except Exception, err:
                log.error('Error send data to %s: %s' % (server, err.message))
            finally:
                if sock:
                    sock.close()
        else:
            log.error('Can\'t connect to %s (%s)' % (server, fail_msg))
            mon_stat['failed_srv'].append(server)

    else:
        log.error('Can\'t resolve ip for %s. Don\'t send data' % server)

    # Write not sended data to file
    if sended_data:
        log.debug('Write not sended data to %s' % data_file_name)
        sended_data.reverse()
        data_fl = None
        try:
            data_fl = open(data_file_name, 'w')
            for line in sended_data:
                data_fl.write(line+'\n')
        except Exception, err:
            log.error('Error write data to %s: %s' % \
                    (data_file_name, err.message))
        finally:
            if data_fl:
                data_fl.close()
    else:
        if os.path.exists(data_file_name) and os.path.isfile(data_file_name):
            log.info('Remove old datafile %s' % data_file_name)
            os.remove(data_file_name)

    return resolve_time, connect_time, send_time

if __name__ == '__main__':
    main()
    sys.exit(0)

