import Tokenparser
import time

def mirror_parser(data):
    p = Tokenparser.Tokenparser()
    #date
    p.skip('[')
    p.upTo('date',' ')
    p.skip(' ')
    # UTC
    p.upTo('utc',']')
    p.skip(']')
    p.skip(' ')
    #http_host
    p.upTo('http_host',' ')
    p.skip(' ')
    #remote_addr
    p.upTo('remote_addr',' ')
    p.skip(' ')
    #method
    p.skip('"')
    p.upTo('method',' ')
    p.skip(' ')
    #geturl
    p.upTo('geturl',' ')
    p.skip(' ')
    #protocol
    p.upTo('protocol','"')
    p.skip('"')
    #status
    p.skip(' ')
    p.upTo('status',' ')
    p.skip(' ')
    #http_referer
    p.skip('"')
    p.upTo('http_referer','"')
    p.skip('"')
    #http_user_agent
    p.skip(' ')
    p.skip('"')
    p.upTo('http_user_agent','"')
    p.skip('"')
    #http_cookie
    p.skip(' ')
    p.skip('"')
    p.upTo('http_cookie','"')
    p.skip('"')
    #request_time
    p.skip(' ')
    p.upTo('request_time',' ')
    #upstream_cache_status
    p.skip(' ')
    p.upTo('upstream_cache_status',' ')
    #bytes_sent
    p.skip(' ')
    p.upTo('bytes_sent',' ')
    #uri
    #p.skip(' ')
    #p.skip('"')
    #p.upTo('uri','"')
    #p.skip('"')
    #args
    #p.skip(' ')
    #p.skip('"')
    #p.upTo('args','"')
    #p.skip('"')
    #ssl_session_id
    #p.skip(' ')
    #p.upTo('ssl_session_id',' ')
    def postprocessing(voc):
        try:
            voc['Time'] = int(time.mktime(time.strptime(voc['date'], "%d/%b/%Y:%H:%M:%S")))
        except ValueError:
            voc['Time'] = 0
        try:
            voc['request_time'] = int(float(voc['request_time'])*1000)
        except ValueError:
            voc['request_time'] = 0
        try:
            voc['method'] = voc['method'].decode('UTF-8')
        except UnicodeDecodeError:
            voc.pop('method')
        try:
            voc['bytes_sent'] = int(voc['bytes_sent'])
            voc['http_status'] = int(voc['http_status'])
        except:
            pass
        return voc
    return (postprocessing(_item) for _item in p.multilinesParse(data))
