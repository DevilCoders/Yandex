# -*- coding: utf-8 -*-
from devtools import fleur
from devtools.fleur.ytest import suite, test, generator
from devtools.fleur.ytest.tools.asserts import *
from antirobot.daemon.test.AntirobotTestSuite import AntirobotTestSuite
from antirobot.daemon.test.util.asserts import *
from antirobot.daemon.test.util import GenRandomIP, IsCaptchaRedirect, Fullreq
from antirobot.daemon.test.util.req_sender import RequestSender
import os
import re
import time
import socket
import itertools
from datetime import datetime, timedelta
from contrib.python.retrying.retrying import retry

class Crawler:
    def __init__(self, ip, userAgent, hostRegExp):
        self.Ip = ip
        self.UserAgent = userAgent
        self.HostRe = re.compile(hostRegExp)
        self.Request = Fullreq("http://yandex.ru/search", headers={
            "User-Agent" : self.UserAgent,
            "X-Forwarded-For-Y" : self.Ip,
        })

GOOGLE = Crawler(ip="66.249.64.190",
    userAgent=r"Mozilla/5.0 (compatible; Googlebot/2.1; +http://www.google.com/bot.html)",
    hostRegExp=r".+\.googlebot\.com")

PINGDOM = Crawler(ip="94.75.211.73",
    userAgent=r"Pingdom.com_bot_version_1.4_(http://www.pingdom.com)",
    hostRegExp=r".+\.pingdom\.com")

APPLE_BOT = Crawler(ip="17.58.98.192",
    userAgent=r"Mozilla/5.0 (Macintosh; Intel Mac OS X 10_10_1) AppleWebKit/600.2.5 (KHTML, например Gecko) Version/8.0.2 Safari/600.2.5 (Applebot/0.1)",
    hostRegExp=r".+\.applebot\.apple\.com")

YAHOO = Crawler(ip="74.6.168.189",
    userAgent=r"Mozilla/5.0 (compatible; Yahoo! Slurp; http://help.yahoo.com/help/us/ysearch/slurp)",
    hostRegExp=r".+\.crawl\.yahoo\.net")

SPUTNIK = Crawler(ip="5.143.231.45",
    userAgent=r"Mozilla/5.0 (compatible; SputnikBot/2.3; +http://corp.sputnik.ru/webmaster)",
    hostRegExp=r"spider.+\.sputnik\.ru")

SEZNAM = Crawler(ip="77.75.79.32",
    userAgent=r"Mozilla/5.0 (compatible; SeznamBot/3.2; +http://napoveda.seznam.cz/en/seznambot-intro/)",
    hostRegExp=r"fulltextrobot.*\.seznam\.cz")

OK_BOT = Crawler(ip="217.20.153.217",
    userAgent=r"Mozilla/5.0 (compatible; OdklBot/1.0 like Linux; klass@odnoklassniki.ru)",
    hostRegExp=r".+\.odnoklassniki\.ru")

# retry нужен потому что DNS порой сбоит и из gethostbyaddr бросается исключение
@retry(stop_max_attempt_number=600, wait_fixed=100)
def AssertSearchEngineCrawler(crawler):
    hostname = socket.gethostbyaddr(crawler.Ip)[0]
    AssertTrue(crawler.HostRe.match(hostname), "{} expected to match regexp {}".format(hostname, crawler.HostRe.pattern))

    resolveResult = socket.getaddrinfo(hostname, 0)
    hostIps = [rec[4][0] for rec in resolveResult]
    AssertContains(hostIps, crawler.Ip)

def FillRecongnizedCrawlersFile(filename, crawlerIps, liveUntil):
    with open(filename, "wt") as f:
        for ip in crawlerIps:
            print >>f, '%s\t%d' % (ip, time.mktime(liveUntil.timetuple()) * 1e6)


@suite(package="antirobot.daemon")
class SearchEngineCrawlersRecognition(AntirobotTestSuite):
    def SetupSuite(self):
        # Мы используем IP одного из краулеров Google. Перед запуском тестов надо
        # проверить, что этот IP всё ещё принадлежит краулеру Google.
        for c in [GOOGLE, PINGDOM, APPLE_BOT, YAHOO, SPUTNIK, SEZNAM]:
            AssertSearchEngineCrawler(c)

        super(SearchEngineCrawlersRecognition, self).SetupSuite()

    def StartCrawler(self, crawler, daemon):
        return RequestSender(suite=daemon, threadCount=5,
                             requestGenerator=lambda : itertools.repeat(crawler.Request))

    def MakeCrawlersKnownAtStartup(self, crawlerIps, crawlerLiveTime):
        runtimeDir = self.GetWorkPath("runtime")
        fleur.util.path.MakeDir(runtimeDir, removeExisting=True)
        recognizedCrawlersFile = "crawlers.lst"

        FillRecongnizedCrawlersFile(os.path.join(runtimeDir, recognizedCrawlersFile),
                                   crawlerIps,
                                   datetime.now() + crawlerLiveTime)

        return {
            "RuntimeDataDir" : runtimeDir,
            "SearchBotsFile" : recognizedCrawlersFile,
            "SearchBotsLiveTime" : "%ss" % crawlerLiveTime.total_seconds(),
            'random_bans_fraction' : 0,
        }

    @generator([
        GOOGLE,
        PINGDOM,  # Pingdom тестирует, что мы корректно обрабатываем IPv6
        APPLE_BOT,
        YAHOO,
        SPUTNIK,
        SEZNAM,
    ])
    def CrawlerUnknownAtStatup(self, crawler):
        config = {
            'random_bans_fraction' : 0,
        }
        with self.StartDaemon(config) as d, self.StartCrawler(crawler, daemon=d):
            # До того, как мы распознали краулера, мы можем его забанить
            AssertEventuallyTrue(lambda : IsCaptchaRedirect(d.SendRequest(crawler.Request)))

            d.SendRequest("/admin?action=reloaddata")

            # После reloaddata краулер должен распознаться, и его должно тут же разбанить
            AssertNotRobotResponse(d.SendRequest(crawler.Request))

            # Кроме того, после reloaddata должен появиться файл с адресами распознанных краулеров
            recognizedCrawlersFile = d.DumpCfg()["SearchBotsFile"]
            AssertPathExists(recognizedCrawlersFile)
            recognizedCrawlers = [line.split('\t')[0] for line in open(recognizedCrawlersFile)]
            AssertContains(recognizedCrawlers, crawler.Ip)

    @test
    def CrawlerKnownAtStatup(self):
        crawler = GOOGLE

        daemonConfig = self.MakeCrawlersKnownAtStartup([crawler.Ip], timedelta(seconds=60))
        with self.StartDaemon(daemonConfig) as d, self.StartCrawler(crawler, daemon=d):
            # Даём время на то, чтобы Ловилка обработала запросы от краулера
            time.sleep(1)

            # Если мы знаем краулера при запуске, мы сразу должны его не банить
            AssertNotRobotResponse(d.SendRequest(crawler.Request))

    @test
    def ForgetIfDoesntCome(self):
        crawler = GOOGLE

        crawlerLiveTime = timedelta(seconds=2)
        daemonConfig = self.MakeCrawlersKnownAtStartup([crawler.Ip], crawlerLiveTime)
        with self.StartDaemon(daemonConfig) as d:
            # Ждём время, в течение которого мы помним crawler'а
            time.sleep(crawlerLiveTime.total_seconds())

            # reloaddata нужна, чтобы отфильтровать просроченных crawler'ов
            d.SendRequest("/admin?action=reloaddata")

            with self.StartCrawler(crawler, daemon=d):
                AssertEventuallyTrue(lambda : IsCaptchaRedirect(d.SendRequest(crawler.Request)))

    @test
    def DontForgetIfComes(self):
        crawler = GOOGLE

        crawlerLiveTime = timedelta(seconds=2)
        daemonConfig = self.MakeCrawlersKnownAtStartup([crawler.Ip], crawlerLiveTime)
        with self.StartDaemon(daemonConfig) as d:
            # Краулер пришёл
            d.SendRequest(crawler.Request)

            # Ждём время, в течение которого мы помним crawler'а
            time.sleep(crawlerLiveTime.total_seconds())

            # reloaddata нужна, чтобы отфильтровать просроченных crawler'ов
            d.SendRequest("/admin?action=reloaddata")

            with self.StartCrawler(crawler, daemon=d):
                # Даём время на то, чтобы Ловилка обработала запросы от краулера
                time.sleep(1)

                # Мы должны помнить краулера и поэтому не банить
                AssertNotRobotResponse(d.SendRequest(crawler.Request))

    @test
    def IsRecognisedOnBothCacherAndProcessor(self):
        crawler = GOOGLE

        ports = [self.PortGen.next() for _ in range(2)]
        processPorts = [self.PortGen.next() for _ in range(2)]

        # Здесь мы делаем небольшой хак. Мы запускаем два антиробота - cacher и processor, но в
        # AllDaemons cacher'а мы указываем только processor. Так мы гарантируем, что все запросы
        # с cacher'а будут пересылаться на processor.
        cacher = self.StartDaemon({
            "Port" : ports[0],
            "ProcessServerPort" : processPorts[0],
            "AllDaemons" : "localhost:%d" % processPorts[1],
        })
        processor = self.StartDaemon({
            "Port" : ports[1],
            "ProcessServerPort" : processPorts[1],
        })

        with self.StartCrawler(crawler, daemon=cacher):
            time.sleep(0.1)

        for d in (cacher, processor):
            d.SendRequest("/admin?action=reloaddata")

        for d in (cacher, processor):
            with open(d.DumpCfg()["SearchBotsFile"], "rt") as crawlersDump:
                AssertContains(crawlersDump.read(), crawler.Ip)
