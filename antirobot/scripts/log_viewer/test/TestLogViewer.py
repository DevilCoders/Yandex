# -*- coding: utf-8 -*-
import datetime

from google.protobuf import symbol_database as db

from devtools.fleur import util
from devtools.fleur.ytest import suite, test, generator, TestSuite
from devtools.fleur.ytest.tools.asserts import AssertTrue, AssertEqual, AssertContains
from devtools.fleur.util.yt_launcher import DownloadYT, YtLauncher

from antirobot.scripts.log_viewer.application import app, ytInst
from antirobot.scripts.log_viewer.config import Config
from antirobot.scripts.log_viewer import eventlog_adapter
from antirobot.scripts.antirobot_eventlog import event as antirobot_events

from yt import wrapper as yt

import dummy_events as D

AllEvents = {
    'TAntirobotFactors': D.AntirobotFactors,
    'TCaptchaRedirect': D.CaptchaRedirect,
    'TCaptchaShow': D.CaptchaShow,
    'TCaptchaCheck': D.CaptchaCheck,
    'TBadRequest': D.BadRequest,
    'TRequestData': D.RequestData,
    'TCaptchaTokenExpired': D.CaptchaTokenExpired,
    'TCaptchaImageShow': D.CaptchaImageShow,
    'TCaptchaImageError': D.CaptchaImageError,
    'TRequestGeneralMessage': D.RequestGeneralMessage,
    'TGeneralMessage': D.GeneralMessage,
    'TWizardError': D.WizardError,
    'TBlockEvent': D.BlockEvent,
    'TCaptchaVoice': D.CaptchaVoice,
    'TCaptchaVoiceIntro': D.CaptchaVoiceIntro,
    'TCbbRuleParseResult': D.CbbRuleParseResult,
    'TCbbRulesUpdated': D.CbbRulesUpdated,
    }

DisplayedEventTypes = [name for name in AllEvents.iterkeys() if name not in eventlog_adapter.IGNORED_EVENT_TYPES]


@suite(package="antirobot.scripts")
class LogViewer(TestSuite):
    DATE = datetime.date(2017, 3, 20)

    def SetupSuite(self):
        conf = Config()

        self.appClient = app.test_client()
        app.config['TESTING'] = True
        app.testing = True
        conf.WORK_DIR = self.GetWorkPath()
        app.config['MY_CONFIG'] = conf

        self.YtRootPath = self.GetWorkPath('YT_LOCAL')
        DownloadYT(self.YtRootPath)

        self.YtLauncher = YtLauncher(self.YtRootPath, conf.WORK_DIR)
        if not self.YtLauncher.Start():
            raise Exception("Could not start local YT")

        yt.config['proxy']['url'] = self.YtLauncher.YtProxy
        self.PrepareYt()

        conf.YT_PROXY = self.YtLauncher.YtProxy
        ytInst.config['proxy']['url'] = self.YtLauncher.YtProxy

    def TeardownSuite(self):
        self.app = None
        self.YtLauncher.Stop()

    def PrepareYt(self):
        StrDate = lambda d: d.strftime("%Y-%m-%d")
        strDate = StrDate(self.DATE)

        yt.create("table", "//logs/antirobot-binary-event-log/1d/%s" % strDate, recursive=True)
        yt.create("map_node", "//home/antirobot/log-viewer/antirobot-binary-event-log_ip", recursive=True)
        yt.create("map_node", "//home/antirobot/tmp", recursive=True)

        self.DayTable = '//logs/antirobot-binary-event-log/1d/%s' % strDate
        self.PrecalcedDayTable = '//home/antirobot/log-viewer/antirobot-binary-event-log_ip/%s' % strDate

        yt.write_table(self.DayTable,
            [{'event': e.SerializeToString().decode('latin1')} for e in self.MakeEventsForTable()], format='json')

        yt.write_table(self.PrecalcedDayTable,
            [{
                'ip': D.DEFAULT_IP,
                'timestamp': e.Timestamp,
                'row_index': i,
                'table': None,
             } for i, e in enumerate(self.MakeEventsForTable())
            ], format='json')
        yt.run_sort(self.PrecalcedDayTable, sort_by=['ip', 'timestamp'])

    def MakeEventsForTable(self):
        for evName, evFunc in AllEvents.iteritems():
            yield evFunc()
            #yield {'event': event.SerializeToString() }

    @test
    def YtTableHasAllEventTypes(self):
        # Ensure that all event types were all put to tables
        AssertEqual(yt.row_count(self.DayTable), len(AllEvents))
        AssertEqual(yt.row_count(self.PrecalcedDayTable), len(AllEvents))

    @test
    def MordaAcccessLog(self):
        resp = self.appClient.get('/')
        AssertEqual(resp.status_code, 200)
        AssertTrue('Search by accesslog' in resp.data)

    @test
    def MordaMarketAcccessLog(self):
        resp = self.appClient.get('/accesslog/market')
        AssertEqual(resp.status_code, 200)
        AssertTrue("Search by market's accesslog" in resp.data)

    @test
    def MordaEventLog(self):
        resp = self.appClient.get('/eventlog')
        AssertEqual(resp.status_code, 200)
        AssertTrue("Search by antirobot's eventlog" in resp.data)


    @test
    def AllEventsTested(self):
        AssertEqual(len(AllEvents), len(antirobot_events.GetAntirobotEventClasses()))


    @test
    def AllEventsHandled(self):
        url = '/ajax/eventlog?date=%s&ip=%s&yandexuid=&substring=&token=&doreq=1&csrf_token=1490367722%23%238556baa9505098efa6a60f2058fbcfae2e9ee906' \
                % (self.DATE.strftime('%Y%m%d'), D.DEFAULT_IP)

        resp = self.appClient.get(url)
        AssertEqual(200, resp.status_code)

        for eventName in DisplayedEventTypes:
            AssertContains(resp.data, eventName)
