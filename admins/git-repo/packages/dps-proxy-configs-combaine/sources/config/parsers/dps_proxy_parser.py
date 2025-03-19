import Tokenparser
import time

def dps_proxy_parser(data):
    """[21/Feb/2013:15:13:10 +0400] 127.0.0.1:8080 127.0.0.1 "PUT /upload/ksoy%40yandex.ru/20130221151310.843861/67723498911362258029104%40%CE%EB%E5%E3-%CF%CA HTTP/1.1" 200 "-" "PycURL/7.19.5" "-" 0.004 - 813 "0.004" """
    """[09/Dec/2013:14:21:02 +0400] dps-proxy.corba.yandex.net ::ffff:178.154.239.4 "GET /?file=/db/company3/i/prev/s32.png&version=stable HTTP/1.0" 200 "http://company.yandex.ru/" "Mozilla/5.0 (Windows NT 6.0) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/31.0.1650.63 Safari/537.36" "yandexuid=544861101386584384; fuid01=52a599437196f2ae.JZlIsmxwbHoh0OWTGrClk_of17l39ZaMWaaI2E8Yyrrik9U3cpzs_H5BQQ7E6LWsifowb9wsWvsGh9JZhnxXwGiHu6Q5kEZhuUd4xK6HVkdmM6Q8aXx9q-QNJlKmZOkw; yabs-frequency=/4/0W0006MPfL800000/; _ym_visorc=w" 0.007 - 3277 """
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

