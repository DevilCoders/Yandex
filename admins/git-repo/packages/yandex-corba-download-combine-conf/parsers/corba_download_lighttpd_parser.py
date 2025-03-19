import Tokenparser
import time

def corba_download_lighttpd_parser(data):
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
    #bytes_sent
    p.skip(' ')
    p.upTo('bytes_sent',' ')
    def postprocessing(voc):
        try:
            voc['Time'] = int(time.mktime(time.strptime(voc['date'], "%d/%b/%Y:%H:%M:%S")))
        except ValueError:
            voc['Time'] = 0
        try:
            voc['request_time'] = int(voc['request_time'])
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
