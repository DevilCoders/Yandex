#!/usr/bin/python
# -*- coding: utf-8 -*-


import codecs
import re
import sys
import time
import urllib
import urllib2
import uuid
import xml.sax.saxutils
import zlib

from loghandlers import SteamLogger


ZLIB_MAX_WBITS = 15
GZIP_HEADER = 16


class Query:

    text = ""
    region = 213
    url = ""

    def __init__(self, query_dict={"text": "",
                                   "region": 213,
                                   "url": ""}):
        self.__dict__ = query_dict
        self.region = int(self.region)
        if self.url != "":
            self.text += " url:%s" % self.url

    def get_xml(self):
        return u"""<ns2:query>
                       <ns2:region-id>%d</ns2:region-id>
                       <ns2:text><![CDATA[%s]]></ns2:text>
                   </ns2:query>""" % (self.region,
                                      xml.sax.saxutils.escape(codecs.decode(self.text, "utf-8")))


class Downloader:

    queries = []
    login = "evilenka"
    results_on_page = 10
    pages = 1
    host = "hamster.yandex.ru"
    params = ""
    headers = {
        "User-Agent": "Mozilla/5.0 (X11; U; Linux i686; en-US; rv:1.8.1.13) Gecko/20080325 Ubuntu/7.10 (gutsy) Firefox/2.0.0.13",
        "Accept": "text/xml,application/xml,application/xhtml+xml,text/html;q=0.9,text/plain;q=0.8,image/png,*/*;q=0.5",
        "Accept-Language": "ru,en-us;q=0.7,en;q=0.3",
        "Accept-Encoding": "gzip,deflate",
        "Accept-Charset": "ISO-8859-1,utf-8;q=0.7,*;q=0.7",
        "Connection": "close",
        "Content-Type": "application/soap+xml;charset=utf-8"
    }
    ticket = ""
    event_uid = ""

    def __init__(self, login="evilenka", results_count=10, pages_count=1,
                 host="hamster.yandex.ru", params_string=""):
        self.queries = []
        self.login = login
        self.results_on_page = results_count
        self.pages = pages_count
        self.host = host
        self.params = params_string
        self.event_uid = uuid.uuid4()

    def exit_by_sigterm(self):
        if self.ticket:
            if self.stop_download():
                SteamLogger.info("Download %(event_uid)s stopped successfully",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_EVENT")
            else:
                SteamLogger.warning("Could not stop download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_WARNING")
        SteamLogger.info("Download %(event_uid)s terminated",
                         event_uid=self.event_uid, type="SERP_DOWNLOAD_TERMINATED")
        exit()

    def add_query(self, query):
        self.queries.append(query)

    def get_request_xml(self):
        if len(self.queries):
            queries_elem = u"".join((query.get_xml() for query in self.queries))
            return u"""<ns1:createAndStartDownloadByParams xmlns:ns1="http://ang-downloader.yandex.ru">
                           <ns1:download-params xmlns:ns2="http://ang-downloader.yandex.ru/downloader">
                               <ns2:login>%s</ns2:login>
                               <ns2:queries>%s</ns2:queries>
                               <ns2:host>%s</ns2:host>
                               <ns2:type>YANDEX</ns2:type>
                               <ns2:pages-count>%d</ns2:pages-count>
                               <ns2:results-on-page-count>%d</ns2:results-on-page-count>
                               <ns2:cgi-params>%s</ns2:cgi-params>
                               <ns2:description>Steam SERP downloader</ns2:description>
                           </ns1:download-params>
                       </ns1:createAndStartDownloadByParams>""" % (self.login,
                                                                   queries_elem,
                                                                   self.host,
                                                                   self.pages,
                                                                   self.results_on_page,
                                                                   xml.sax.saxutils.escape(self.params))
        SteamLogger.error("No queries specified for download %(event_uid)s", event_uid=self.event_uid,
                          type="SERP_DOWNLOAD_START_ERROR")
        raise Exception("No queries specified")

    def ask_soap_server(self, xml):
        request = urllib2.Request(url="http://serp-downloader.yandex-team.ru/services/soap-downloader",
                                      headers=self.headers)
        request_data = """<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" xmlns:xsd="http://www.w3.org/2001/XMLSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance">
                               <soap:Body>%s</soap:Body>
                           </soap:Envelope>""" % codecs.encode(xml, "utf-8")
        response = urllib2.urlopen(request, request_data)
        return response

    def switch_uid(self, uid):
        self.event_uid = uid

    def start_download(self):
        SteamLogger.info("Called createAndStartDownloadByParams function for download %(event_uid)s",
                         event_uid=self.event_uid, type="SERP_DOWNLOAD_START_EVENT")
        try:
            response = self.ask_soap_server(self.get_request_xml())
        except urllib2.HTTPError as err:
            details = {"event_uid": self.event_uid, "type": "SERP_DOWNLOAD_START_ERROR",
                       "response": err.read(), "HTTP_code": err.code, "HTTP_msg": err.msg}
            message = "HTTP error %(HTTP_code)d: %(HTTP_msg)s while starting download %(event_uid)s"
            fault_re = re.compile(r"<faultstring>((?:.|\s)*?)</faultstring>")
            try:
                fault = fault_re.search(details["response"]).group(1)
                details["SOAP_faultstring"] = fault
                message += ", %(SOAP_faultstring)s"
            except (KeyboardInterrupt, SystemExit):
                raise
            except:
                pass
            SteamLogger.error(message, **details)
            raise
        except Exception:
            SteamLogger.error("Could not get response while starting download %(event_uid)s",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_START_ERROR")
            raise
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            SteamLogger.error("Unknown error while starting download %(event_uid)s",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_START_ERROR")
            raise
        try:
            self.get_ticket(response)
        except:
            SteamLogger.error("Could not get ticket for download %(event_uid)s",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_START_ERROR")
            raise

    def get_ticket(self, response):
        ticket_re = re.compile(r"<ns1:download-ticket>.*?<ns2:download-id.*?>(.*?)</ns2:download-id>.*?</ns1:download-ticket>")
        match = ticket_re.search(response.read())
        self.ticket = match.group(0)
        ticket_id = match.group(1)
        try:
            response.close()
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            SteamLogger.warning("Could not close response stream while starting download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_START_WARNING")
        self.ticket = self.ticket.replace("ns2:", "").replace(":ns2", "")
        SteamLogger.info("HTTP response %(HTTP_code)d: %(HTTP_msg)s, Got ticket %(ticket)s for download %(event_uid)s",
                         event_uid=self.event_uid, type="SERP_DOWNLOAD_START_EVENT",
                         HTTP_code=response.code, HTTP_msg=response.msg, ticket=ticket_id)

    def is_finished(self):
        xml = """<ns1:isFinished xmlns:ns1="http://ang-downloader.yandex.ru">
                     <ns1:login>%s</ns1:login>
                     %s
                 </ns1:isFinished>""" % (self.login, self.ticket)
        try:
            response = self.ask_soap_server(xml)
        except urllib2.HTTPError as err:
            details = {"event_uid": self.event_uid, "type": "SERP_DOWNLOAD_WARNING",
                       "response": err.read(), "HTTP_code": err.code, "HTTP_msg": err.msg}
            message = "HTTP error %(HTTP_code)d during isFinished for download %(event_uid)s: %(HTTP_msg)s"
            fault_re = re.compile(r"<faultstring>((?:.|\s)*?)</faultstring>")
            try:
                fault = fault_re.search(details["response"]).group(1)
                details["SOAP_faultstring"] = fault
                message += ", %(SOAP_faultstring)s"
            except (KeyboardInterrupt, SystemExit):
                raise
            except:
                pass
            SteamLogger.warning(message, **details)
        except Exception:
            SteamLogger.warning("Could not get response for isFinished for download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_WARNING")
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            SteamLogger.warning("Unknown error during isFinished for download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_WARNING")
        try:
            finished_re = re.compile(r"<ns1:is-finished>\s*(true|false)\s*</ns1:is-finished>")
            finished = finished_re.search(response.read()).group(1)
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            SteamLogger.warning("Could not determine if download %(event_uid)s is finished",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_WARNING")
        try:
            response.close()
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            SteamLogger.warning("Could not close response stream during isFinished for download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_WARNING")
        if finished == 'true':
            SteamLogger.info("HTTP response %(HTTP_code)d: %(HTTP_msg)s, download %(event_uid)s finished",
                             event_uid=self.event_uid, type="SERP_DOWNLOAD_EVENT",
                             HTTP_code=response.code, HTTP_msg=response.msg)
            return True
        return False

    def stop_download(self):
        SteamLogger.info("Called stopDownload function for download %(event_uid)s",
                         event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_EVENT")
        xml = """<ns1:stopDownload xmlns:ns1="http://ang-downloader.yandex.ru">
                     <ns1:login>%s</ns1:login>
                     %s
                 </ns1:stopDownload>""" % (self.login, self.ticket)
        try:
            response = self.ask_soap_server(xml)
        except urllib2.HTTPError as err:
            details = {"event_uid": self.event_uid, "type": "SERP_DOWNLOAD_STOP_WARNING",
                       "response": err.read(), "HTTP_code": err.code, "HTTP_msg": err.msg}
            message = "HTTP error %(HTTP_code)d during stopDownload for download %(event_uid)s: %(HTTP_msg)s"
            fault_re = re.compile(r"<faultstring>((?:.|\s)*?)</faultstring>")
            try:
                fault = fault_re.search(details["response"]).group(1)
                details["SOAP_faultstring"] = fault
                message += ", %(SOAP_faultstring)s"
            except:
                pass
            SteamLogger.warning(message, **details)
            return False
        except Exception:
            SteamLogger.warning("Could not get response for stopDownload for download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_WARNING")
            return False
        except:
            SteamLogger.warning("Unknown error during stopDownload for download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_WARNING")
            return False
        try:
            stopped_re = re.compile(r"<ns1:is-stopped>\s*(true|false)\s*</ns1:is-stopped>")
            stopped = stopped_re.search(response.read()).group(1)
        except:
            SteamLogger.warning("Could not determine if download %(event_uid)s is stopped",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_WARNING")
            return False
        try:
            response.close()
        except:
            SteamLogger.warning("Could not close response stream during stopDownload for download %(event_uid)s",
                                event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_WARNING")
        if stopped == 'true':
            SteamLogger.info("HTTP response %(HTTP_code)d: %(HTTP_msg)s, download %(event_uid)s stopped",
                             event_uid=self.event_uid, type="SERP_DOWNLOAD_STOP_EVENT",
                             HTTP_code=response.code, HTTP_msg=response.msg)
            return True
        return False

    def get_gzipped_xml(self):
        xml = """<ns1:getSerpsGZip2 xmlns:ns1="http://ang-downloader.yandex.ru">
                      <ns1:login>%s</ns1:login>
                      %s
                  </ns1:getSerpsGZip2>""" % (self.login, self.ticket)
        try:
            response = self.ask_soap_server(xml)
        except urllib2.HTTPError as err:
            details = {"event_uid": self.event_uid, "type": "SERP_DOWNLOAD_ERROR",
                       "response": err.read(), "HTTP_code": err.code, "HTTP_msg": err.msg}
            message = "HTTP error %(HTTP_code)d during getSerpsGZip2 for download %(event_uid)s: %(HTTP_msg)s"
            fault_re = re.compile(r"<faultstring>((?:.|\s)*?)</faultstring>")
            try:
                fault = fault_re.search(details["response"]).group(1)
                details["SOAP_faultstring"] = fault
                message += ", %(SOAP_faultstring)s"
            except (KeyboardInterrupt, SystemExit):
                raise
            except:
                pass
            SteamLogger.error(message, **details)
            raise
        except Exception:
            SteamLogger.error("Could not get response for getSerpsGzip2 for download %(event_uid)s",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_ERROR")
            raise
        except (KeyboardInterrupt, SystemExit):
            raise
        except:
            SteamLogger.error("Unknown error during getSerpsGzip2 for download %(event_uid)s",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_ERROR")
            raise
        if ("content-type" in response.headers.dict and
            "multipart" in response.headers.dict["content-type"]):
            try:
                boundary_re = re.compile(r'boundary="(.*?)"')
                boundary = boundary_re.search(response.headers.dict["content-type"]).group(1)
            except (KeyboardInterrupt, SystemExit):
                raise;
            except:
                SteamLogger.error("Could not locate gzip part boundary for download %(event_uid)s",
                                  event_uid=self.event_uid, type="SERP_DOWNLOAD_ERROR")
                raise Exception("Could not locate gzip part boundary")
            parts = response.read().split("".join(("--", boundary)))[1:-1]
            try:
                response.close()
            except (KeyboardInterrupt, SystemExit):
                raise
            except:
                SteamLogger.warning("Could not close response stream during getSerpsGzip2 for download %(event_uid)s",
                                    event_uid=self.event_uid, type="SERP_DOWNLOAD_WARNING")
            for part in parts:
                part = re.sub(r"^\r\n|\r\n$", "", part)
                try:
                    part_headers, part_body = part.split("\r\n\r\n")
                except (KeyboardInterrupt, SystemExit):
                    raise
                except:
                    part_body = ""
                part_headers = dict(re.findall(r"(.*?): (.*?)\r\n", part_headers + "\r\n"))
                if ("Content-Type" in part_headers and
                    part_headers["Content-Type"] == "application/x-gzip"):
                    SteamLogger.info("HTTP response %(HTTP_code)d: %(HTTP_msg)s, Got gzipped SERP for download %(event_uid)s",
                                     event_uid=self.event_uid, type="SERP_DOWNLOAD_EVENT",
                                     HTTP_code=response.code, HTTP_msg=response.msg)
                    return part_body
            SteamLogger.error("Could not locate gzip part for download %(event_uid)s",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_ERROR")
            raise Exception("Could not locate gzip part")
        else:
            try:
                response.close()
            except (KeyboardInterrupt, SystemExit):
                raise
            except:
                SteamLogger.warning("Could not close response stream during getSerpsGzip2 for download %(event_uid)s",
                                    event_uid=self.event_uid, type="SERP_DOWNLOAD_WARNING")
            SteamLogger.error("Could not locate gzip part for download %(event_uid)s",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_ERROR")
            raise Exception("Could not locate gzip part")

    def download(self, filename):
        try:
            self.start_download()
            while not self.is_finished():
                time.sleep(1)
            gzipped_xml = self.get_gzipped_xml()
            self.ticket = ""
        except (KeyboardInterrupt, SystemExit):
            self.exit_by_sigterm()
        except:
            SteamLogger.error("Download %(event_uid)s failed",
                              event_uid=self.event_uid, type="SERP_DOWNLOAD_ERROR")
            return False
        SteamLogger.info("Decompressing download %(event_uid)s",
                         event_uid=self.event_uid, type="SERP_DECOMPRESS_EVENT")
        try:
            ungzipped = zlib.decompress(gzipped_xml, ZLIB_MAX_WBITS + GZIP_HEADER)
        except (KeyboardInterrupt, SystemExit):
            self.exit_by_sigterm()
        except:
            SteamLogger.error("Decompressing download %(event_uid)s failed",
                              event_uid=self.event_uid, type="SERP_DECOMPRESS_ERROR")
            return False
        SteamLogger.info("Decompressing download %(event_uid)s succeeded",
                         event_uid=self.event_uid, type="SERP_DECOMPRESS_EVENT")
        error_re = re.compile(r'<grab-state[^>]*? grab-status="FAILED"[^>]*? error="(.*?")')
        error_match = error_re.match(ungzipped)
        if error_match:
            SteamLogger.error("Serp in download %(event_uid)s is corrupt: %(err_text)s",
                              event_uid=self.event_uid, type="SERP_VALIDATION_ERROR",
                              err_text=error_match.group(1))
            return False
        SteamLogger.info("Storing download %(event_uid)s to file %(file_name)s",
                         event_uid=self.event_uid, type="SERP_WRITE_EVENT",
                         file_name=filename)
        try:
            xml_file = open(filename, "w")
            xml_file.write(ungzipped)
            xml_file.close()
        except (KeyboardInterrupt, SystemExit):
            self.exit_by_sigterm()
        except:
            SteamLogger.error("Storing download %(event_uid)s to file %(file_name)s failed",
                              event_uid=self.event_uid, type="SERP_WRITE_ERROR",
                              file_name=filename)
            return False
        SteamLogger.info("Storing download %(event_uid)s to file %(file_name)s succeeded",
                         event_uid=self.event_uid, type="SERP_WRITE_EVENT",
                         file_name=filename)
        SteamLogger.info("Download %(event_uid)s to file %(file_name)s completed",
                         event_uid=self.event_uid, type="SERP_DOWNLOAD_SUCCESS",
                         file_name=filename)
        return True


def main():
    if len(sys.argv) != 2:
        print >> sys.stderr, "Specify output XML filename"
        exit()
    former = Downloader()
    queries = [{"text": "борьба с тараканами"},
               {"text": "котики"}]
    for query in queries:
        former.add_query(Query(query))
    if former.download(sys.argv[1]):
        print("OK")
    else:
        print("FAIL")


if __name__ == '__main__':
    main()

