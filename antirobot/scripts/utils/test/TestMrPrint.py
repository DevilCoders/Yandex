# -*- coding: utf-8 -*-

from devtools.fleur.ytest import suite, generator
from devtools.fleur.ytest import AssertEqual, AssertNotEmpty
from devtools.fleur import util
from devtools.fleur.util.mapreduce import MapReduce, Record

from antirobot.daemon.test.AntirobotTestSuite import AntirobotTestSuite
from antirobot.scripts.utils import mr_print

from cStringIO import StringIO

DATA = [
    {"key" : "1", "subkey" : "a", "value" : "abcd"},
    {"key" : "2", "subkey" : "b", "value" : "  abcd  "},
    {"key" : "2", "subkey" : "c", "value" : "  ab  cd  "},
    {"key" : "3", "subkey" : "c", "value" : "qwerty"},
]

TABLE = "table"

@suite(package="antirobot.scripts.utils")
class MrPrint(AntirobotTestSuite):
    def SetupSuite(self):
        super(MrPrint, self).SetupSuite()

        MapReduce.useDefaults(server="local", mrExec=self.GetMapreduceBin(), workDir=".", silentLocal=True)

        records=[Record(*[d[key] for key in "key", "subkey", "value"]) for d in DATA]
        MapReduce.updateTable(records, dstTable=TABLE)

    @generator(['sakura', 'cedar', 'redwood'])
    def GetProxyWorks(self, server):
        AssertNotEmpty(mr_print.GetProxy(server))

    @generator([False, True])
    def PrintMrTableSubkey(self, usingSubkey):
        stream = StringIO()
        mr_print.PrintMrTable(TABLE, stream, "local", usingSubkey)

        result = [line for line in stream.getvalue().rstrip().split('\n')]
        keys = ["key", "subkey", "value"] if usingSubkey else ["key", "value"]
        testData = ["\t".join([d[key] for key in keys]) for d in DATA]

        AssertEqual(len(result), len(testData))
        AssertEqual(sorted(result), sorted(testData))

    @generator([False, True])
    def PrintMrTableStrip(self, strip):
        stream = StringIO()
        mr_print.PrintMrTable(TABLE, stream, "local", lstrip=strip)

        result = [line for line in stream.getvalue().rstrip().split('\n')]
        if strip:
            testData = ["\t".join([d[key].strip() for key in "key", "value"]) for d in DATA]
        else:
            testData = ["\t".join([d[key] for key in "key", "value"]) for d in DATA]

        AssertEqual(len(result), len(testData))
        AssertEqual(sorted(result), sorted(testData))
