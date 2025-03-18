#!/usr/bin/env python

import socket
import types
import logging

import tools.ygetparam.ygetparam as ygetparam
from devtools.fleur.ytest import suite, test, generator, TestSuite
from devtools.fleur.ytest.tools import AssertEqual, AssertRaises, AssertCollectionsEquivalent


@suite
class ExtractParamFromDictionary(TestSuite):

    def SetupSuite(self):
        super(ExtractParamFromDictionary, self).SetupSuite()
        self.my_host = socket.gethostname()
        self.manifest = {
            u'component': {
                u'param': u'value',
                u'param2': u'param3',
                u'param3': u'server_name',
                u'@@(param4)': {u'param2': u'value3'},
                u'param4': self.my_host,
                self.my_host: {u'param3': "BINGO"},
                u'param5': [u'item1', u'item2', u'item3'],
            },
            u'component2': {
                u'param6': u'@@(^component.param)',
                u'param5': u'@@(param4)',
                u'param4': u'value4',
                u'@@(param4)': u'test',
                u'@@(^component.param4)': {
                    u'param2': u'@@(^component.@@(^component.param2))',
                },
            },
            u'param1': u'value1',
            self.my_host: {u'param1': u'value101'},
        }

    @test
    def GetStraightforwardParam(self):
        AssertEqual(ygetparam.GetParam(self.manifest, "component.param"), "value")

    @test
    def GetHostDependantParam(self):
        AssertEqual(ygetparam.GetParam(self.manifest, "component.param3"), "BINGO")

    @test
    def GetMultiwayDefinedHostDependantParam(self):
        for key, expected in [
                ("component.param3", "BINGO"),
                ("component.param2", "value3"),
                ("component."+self.my_host+".param3", "BINGO"),
                ("param1", "value101"),
                (self.my_host+".param1", "value101"),
                ]:
            AssertEqual(ygetparam.GetParam(self.manifest, key), expected, "key: "+key)

    @test
    def GetResolvedVarNameVariableForDict(self):
        AssertEqual(ygetparam.GetParam(self.manifest, "component2.param2"), "BINGO")

    @test
    def GetResolvedVarNameVariableForString(self):
        AssertEqual(ygetparam.GetParam(self.manifest, "component2.value4"), "test")

    @test
    def GetResolvedVarList(self):
        result = (u'item1', u'item2', u'item3')
        AssertEqual(ygetparam.GetParam(self.manifest, "component.param5"), result)


@suite
class Compile(TestSuite):

    def SetupSuite(self):
        super(Compile, self).SetupSuite()
        self.manifest = {
            u'component': {
                u'param': u'value',
                u'param2': u'value2',
            },
            u'component2': {
                u'param3': {
                    u'param2': u'value3'
                },
                u'param4': u'value4',
            }
        }

    @test
    def CompileSimpleList(self):
        result = {
            u'self': '',
            u'component.param': u'value',
            u'component.param2': u'value2',
            u'component2.param4': u'value4',
            u'component2.param3.param2': u'value3'
        }
        varList = ygetparam.CompileParamsList(self.manifest)
        AssertEqual(varList, result)

    @test
    def CompileHostDependantList(self):
        self.manifest['component'][socket.gethostname()] = {
            u'param2': u'BINGO'
        }
        result = {
            u'self': '',
            u'component.param': u'value',
            u'component.param2': u'BINGO',
            u'component2.param4': u'value4',
            u'component2.param3.param2': u'value3',
            u'component.'+socket.gethostname()+u'.param2': u'BINGO',
        }
        varList = ygetparam.CompileParamsList(self.manifest)
        AssertEqual(varList, result)


@suite
class Variables(TestSuite):

    def SetupSuite(self):
        super(Variables, self).SetupSuite()
        self.varList = {
            u'component.param': u'value',
            u'component.param1': u'@@(^component2.param4)',
            u'component.param2': u'BINGO',
            u'component.param3': u'@@(^component2.@@(^component2.param7))',
            u'component2.param3.param2': u'value3',
            u'component2.param4': u'value4',
            u'component2.param5': u'param1',
            u'component2.param6': u'@@(^component.param3)',
            u'component2.param7': u'param6',
        }

    @test
    def ResolveNonVariableString(self):
        AssertEqual(
            ygetparam.GetResolvedVariable("test string", self.varList, ""),
            u'test string')
        AssertEqual(
            ygetparam.GetResolvedVariable("", self.varList, "component2"), u'')

    @test
    def ResolveSingleGlobalVariable(self):
        AssertEqual(
            ygetparam.GetResolvedVariable("@@(^component.param)", self.varList, ""),
            u'value')

    @test
    def ResolveSingleLocalVariable(self):
        AssertEqual(
            ygetparam.GetResolvedVariable("@@(param)", self.varList, "component"),
            u'value')

    @test
    def ResolveComplexVariable(self):
        AssertEqual(ygetparam.GetResolvedVariable(
            "@@(param1)", self.varList, "component"
            ), u'value4')
        AssertEqual(ygetparam.GetResolvedVariable(
            "@@(^component.param1)", self.varList, "component"
            ), u'value4')
        AssertEqual(ygetparam.GetResolvedVariable(
            "@@(^component.@@(param5))", self.varList, "component2"
            ), u'value4')
        AssertEqual(ygetparam.GetResolvedVariable(
            "@@(^component.@@(param5))a", self.varList, "component2"
            ), u'value4a')
        AssertEqual(ygetparam.GetResolvedVariable(
            "a@@(^component.@@(param5))a", self.varList, "component2"
            ), u'avalue4a')
        AssertEqual(ygetparam.GetResolvedVariable(
            "a@@(^component.@@(param5))a@@(param5)", self.varList, "component2"
            ), u'avalue4aparam1')
        AssertEqual(ygetparam.GetResolvedVariable(
            "a@@(^component.@@(^component2.param5))a@@(param5)",
            self.varList, "component2"), u'avalue4aparam1')

    @test
    def InfinityRecursion(self):
        AssertRaises(lambda: ygetparam.GetResolvedVariable(
            "@@(param6)", self.varList, "component2"
            ), ygetparam.InfinityRecursionException)

    @test
    def VariableErrors(self):
        AssertRaises(lambda: ygetparam.GetResolvedVariable(
            "@@(param6",
            self.varList, "component2"), ygetparam.MissingClosingLexException)
        AssertRaises(lambda: ygetparam.GetResolvedVariable(
            "a@@(^component.@@(^component2.param5)a@@(param5)",
            self.varList, "component2"), ygetparam.MissingClosingLexException)
        AssertRaises(lambda: ygetparam.GetResolvedVariable(
            "a@@(^component.@@(^component2.param5)a@@(param5",
            self.varList, "component2"), ygetparam.MissingClosingLexException)
        AssertRaises(lambda: ygetparam.GetResolvedVariable(
            "a@@(^component.@@(^component2.param5))a@@(param5",
            self.varList, "component2"), ygetparam.MissingClosingLexException)


@suite
class CalculateManifestFileLocation(TestSuite):
    def SetupSuite(self):
        self.instance = "test_instance"

    @test
    def ManifestFileLocation(self):
        AssertEqual(
            ygetparam.ManifestFileLocation(self.instance),
            "{}{}{}".format(ygetparam.EXTERNAL_STORAGE,
                            ygetparam.LEX_MODULE_CLOSE,
                            self.instance))


@suite
class CompleteParse(TestSuite):
    @test
    def CompileAndResolv(self):
        manifest = {
            u'component': {
                u'param': u'value',
                u'param2': u'value2',
                u'param3': u'server_name',
                u'BINGO': {'param2': u'test'},
                socket.gethostname(): {u'param3': u'BINGO'},
            },
            u'component2': {
                u'param4': u'value4',
                u'param5': u'@@(param4)',
                u'param6': u'@@(^component.param)',
                u'@@(^component.param3)':
                u'@@(^component.@@(^component.param3).param2)',
            },
        }

        result = {
            u'self': '',
            u'component.param': u'value',
            u'component.param2': u'value2',
            u'component.param3': u'BINGO',
            u'component.'+socket.gethostname()+'.param3': u'BINGO',
            u'component.BINGO.param2': u'test',
            u'component2.param6': u'value',
            u'component2.param5': u'value4',
            u'component2.param4': u'value4',
            u'component2.BINGO': u'test',
        }

        AssertEqual(
            ygetparam.ResolveAllVariables(ygetparam.CompileParamsList(manifest)),
            result)


@suite
class ResolveVarsInManifest(TestSuite):
    @test
    def ResolveVarialesInManifest(self):
        manifest = {
            u'component': {
                u'param': u'value',
                u'param2': u'value2',
                u'param3': u'server_name',
                u'BINGO': {
                    'param2': u'test'
                }
            },
            u'component2': {
                u'param4': u'value4',
                u'param5': u'@@(param4)',
                u'param6': u'@@(^component.param)',
                u'param7': [u'item1', u'@@(^component.param2)', u'item3'],
                u'@@(^component.param3)':
                u'@@(^component.@@(^component.param3).param2)'
            }
        }
        manifest['component'][socket.gethostname()] = {
            u'param3': u'BINGO'
        }

        varList = {
            u'component.param': u'value',
            u'component.param2': u'value2',
            u'component.param3': u'BINGO',
            u'component.BINGO.param2': u'test',
            u'component2.param7': [u'item1', u'value2', u'item3'],
            u'component2.param6': u'value',
            u'component2.param5': u'value4',
            u'component2.param4': u'value4',
            u'component2.BINGO': u'test',
        }

        result = {
            u'component': {
                u'BINGO': {
                    'param2': u'test'
                },
                u'param3': u'server_name',
                u'param2': u'value2',
                u'param': u'value'
            },
            u'component2': {
                u'param7': [u'item1', u'value2', u'item3'],
                u'param6': u'value',
                u'param5': u'value4',
                u'param4': u'value4',
                u'BINGO': u'test'
            }
        }
        result['component'][socket.gethostname()] = {u'param3': u'BINGO'}

        AssertEqual(
            ygetparam.GetResolvedManifest(manifest, varList, ""),
            result)


@suite
class ExtractParamBracesExpansion(TestSuite):

    def SetupSuite(self):
        super(ExtractParamBracesExpansion, self).SetupSuite()
        self.manifest = {
            u'self': {
                u'hosts': [u'@@(^component.master.host)', u'@@(^component.worker.host)']
            },
            u'component': {
                u'master': {
                    u'host': u'a0.ya.ru',
                    u'param': u'@@(^a1.ya.ru.command)'
                },
                u'worker': {
                    u'host': u'a{0..3}.ya.ru'
                },
                u'mfas00{0..4}.search.yandex.net': 'test string',
            },
            u'component2': {
                u'param': u'@@(^component.mfas001.search.yandex.net)'
            },
            u'@@(^component.worker.host)': {
                u'command': u'ls -la file{a..c} @@(^component.worker.host)'
            }
        }

    @test
    def GetSingleExpandableValue(self):
        var = "component.master.host"
        result = 'a0.ya.ru'
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)

    @test
    def GetSingleListWithExpandableVariables(self):
        ygetparam.recursionVector = list()
        var = "self.hosts"
        result = ('a0.ya.ru', 'a0.ya.ru', 'a1.ya.ru', 'a2.ya.ru', 'a3.ya.ru')
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)

    @test
    def GetSingleExpandedParam(self):
        ygetparam.recursionVector = list()
        var = "component.mfas001.search.yandex.net"
        result = 'test string'
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)

    @test
    def GetSingleLinkedExpandedParam(self):
        ygetparam.recursionVector = list()
        var = "component2.param"
        result = 'test string'
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)

    @test
    def GetSingleExpandedTopLevelParam(self):
        ygetparam.recursionVector = list()
        var = "a1.ya.ru.command"
        result = 'ls -la filea fileb filec a0.ya.ru a1.ya.ru a2.ya.ru a3.ya.ru'
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)


@suite
class ExtractParamWithPreventedBracesExpansion(TestSuite):

    def SetupSuite(self):
        super(ExtractParamWithPreventedBracesExpansion, self).SetupSuite()
        self.manifest = {
            u'component': {
                u'param': u'plus{0..1}.search.yandex.net',
                u'paramX': [u'@@(param)', u'hostX'],
                u'param2': u'param3',
                u'param3': u'server_name',
                u'@@(param4)': {u'param2': u'value3'},
                u'param5': [u'item1', u'item2', u'item3'],
                u'mfas00{0..4}.search.yandex.net': 'test string',
                u'cmd2': 'ls -la file{1..3} @@(^component.paramX)'
            },
            u'component2': {
                u'param5': u'@@(param4)',
                u'param4': u'value4',
                u'@@(param4)': u'test',
                u'c2-cmd': u'@@(^component.cmd2)',
                u'@@(^component.param4)': {
                    u'param2': u'@@(^component.@@(^component.param2))'
                }
            }
        }
        self.manifest['component'][socket.gethostname()] = {
            u'param3': "BINGO"
        }
        self.manifest['component']['param4'] = socket.gethostname()
        ygetparam.expandCurlyBraces = False

    @test
    def GetSingleExpandableValue(self):
        var = "component.param"
        result = 'plus{0..1}.search.yandex.net'
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)

    @test
    def GetSingleExpandableValueInList(self):
        var = "component.paramX"
        result = ('plus{0..1}.search.yandex.net', "hostX")
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)

    @test
    def GetSingleLinkedExpandableToStringParam(self):
        var = "component2.c2-cmd"
        result = 'ls -la file{1..3} plus{0..1}.search.yandex.net hostX'
        AssertEqual(ygetparam.GetParam(self.manifest, var), result)

    @test
    def GetSingleExpandableValue(self):
        var = "mfas00{0..4}.search.yandex.net"
        AssertRaises(lambda: ygetparam.GetParam(self.manifest, var), ygetparam.BadParamNameException)


@suite
class RaiseOnMultidimensionalListDefinition(TestSuite):

    def SetupSuite(self):
        super(RaiseOnMultidimensionalListDefinition, self).SetupSuite()
        self.manifest = {
            u'component': {
                u'param': u'plus{0..1}.search.yandex.net',
                u'paramX': [u'@@(param)', u'hostX'],
                u'paramY': [
                    [u'@@(param)', u'hostX'],
                    [u'@@(param)', u'hostX']
                ]
            }
        }

    @test
    def CatchMultidimensionalListDefinition(self):
        var = "component.paramX"
        AssertRaises(lambda: ygetparam.GetParam(self.manifest, var), ygetparam.MultidimensionalListException)


class ExtractAllParamsAsEnvVariables(TestSuite):

    def SetupSuite(self):
        super(ExtractAllParamsAsEnvVariables, self).SetupSuite()
        self.manifest = {
            u'component': {
                u'param': u'value',
                u'@@(param3)': {u'param': u'value3'},
                u'param2': [u'item1', u'item2', u'item3']
            }
        }
        self.manifest['component']['param3'] = socket.gethostname()

    @test
    def GetStraightforwardParams(self):
        prefix = ''
        delimiter = ' '
        result = (
            "COMPONENT{}PARAM='value3'".format(ygetparam.LEX_ENV_DELIMITER),
            "COMPONENT{}PARAM2='{}'".format(ygetparam.LEX_ENV_DELIMITER, delimiter.join("item1", "item2", "item3")),
            "COMPONENT{}PARAM3='{}'".format(ygetparam.LEX_ENV_DELIMITER, socket.gethostname()),
        )
        AssertCollectionsEquivalent(ygetparam.GetAllParamsForEnv(self.manifest, delimiter, prefix),
                                    result)

    @test
    def GetStraightforwardParamsWithPrefix(self):
        prefix = 'test'
        delimiter = ' '
        result = (
            "{0}{1}COMPONENT{1}PARAM='value3'".format(prefix.upper(),
                                                      ygetparam.LEX_ENV_DELIMITER),
            "{0}{1}COMPONENT{1}PARAM2='{2}'".format(prefix.upper(),
                                                    ygetparam.LEX_ENV_DELIMITER,
                                                    delimiter.join(("item1", "item2", "item3"))),
            "{0}{1}COMPONENT{1}PARAM3='{2}'".format(prefix.upper(),
                                                    ygetparam.LEX_ENV_DELIMITER,
                                                    socket.gethostname())
        )
        AssertCollectionsEquivalent(GetAllParamsForEnv(self.manifest, delimiter, prefix),
                                    result)


@suite
class ExtractPartOfParametersTree(TestSuite):
    def SetupSuite(self):
        super(ExtractPartOfParametersTree, self).SetupSuite()
        self.manifest = {
            u'component': {
                u'param': u'value',
                u'param2': [u'item1', u'item2', u'item3'],
                u'component2': {
                    u'param': u'value',
                    u'param2': [u'item1', u'item2', u'item3']
                }
            }
        }

    @test
    def GetCompoment(self):
        component = 'component.component2'
        result = {
            u'component2': {
                u'param': u'value',
                u'param2': [u'item1', u'item2', u'item3']
            }
        }
        AssertCollectionsEquivalent(ygetparam.GetPartOfManifest(self.manifest, component),
                                    result)

    @test
    def GetExportedCompoment(self):
        component = 'component.component2'
        prefix = ''
        delimiter = ' '
        result = (
            "COMPONENT{0}COMPONENT2{0}PARAM='value'".format(ygetparam.LEX_ENV_DELIMITER),
            "COMPONENT{0}COMPONENT2{0}PARAM2='{1}'".format(ygetparam.LEX_ENV_DELIMITER,
                                                           delimiter.join(("item1", "item2", "item3")))
        )
        AssertCollectionsEquivalent(ygetparam.GetPartOfParamsForEnv(self.manifest, delimiter, prefix, component),
                                    result)

    @test
    def GetExportedComponentWithPrefix(self):
        component = 'component.component2'
        prefix = 'test'
        delimiter = ' '
        result = (
            "{0}{1}COMPONENT{1}COMPONENT2{1}PARAM='value'".format(prefix.upper(),
                                                                  ygetparam.LEX_ENV_DELIMITER),
            "{0}{1}COMPONENT{1}COMPONENT2{1}PARAM2='{2}'".format(prefix.upper(),
                                                                 ygetparam.LEX_ENV_DELIMITER,
                                                                 delimiter.join(("item1", "item2", "item3")))
        )
        AssertCollectionsEquivalent(ygetparam.GetPartOfParamsForEnv(self.manifest, delimiter, prefix, component),
                                    result)


@suite
class ExternalModule(TestSuite):

    def SetupSuite(self):
        super(ExternalModule, self).SetupSuite()
        self.module = "DUMMY"
        self.string = "Test string"
        self.manifest = {
            u'component': {
                # u'param': u'@@(~DUMMY://Test string)'
                u'param': u"{}{}{}{}{}{}".format(ygetparam.LEX_VAR_OPEN,
                                                 ygetparam.LEX_MODULE_OPEN,
                                                 self.module,
                                                 ygetparam.LEX_MODULE_CLOSE,
                                                 self.string,
                                                 ygetparam.LEX_VAR_CLOSE)
            }
        }

    @test
    def LoadExternalDummyModule(self):
        AssertEqual(hasattr(ygetparam.ExternalModuleLoader(self.module), '__call__'), True)

    @test
    def ResolveVariableViaDummyModule(self):
        AssertEqual(ygetparam.ExternalResolve(self.module, self.string), (self.string,))

    @test
    def ResolveManifestViaDummyModule(self):
        AssertEqual(ygetparam.GetParam(self.manifest, "component.param"), self.string)

    @test
    def ReturnNonTupleViaDummyModule(self):
        self.manifest = {
            u'component': {
                # u'param': u'@@(~DUMMY://return non tuple)'
                u'param': u"{}{}{}{}return non tuple{}".format(ygetparam.LEX_VAR_OPEN,
                                                               ygetparam.LEX_MODULE_OPEN,
                                                               self.module,
                                                               ygetparam.LEX_MODULE_CLOSE,
                                                               ygetparam.LEX_VAR_CLOSE)
            }
        }
        AssertRaises(lambda: ygetparam.GetParam(self.manifest, "component.param"),
                     ygetparam.ExtModResponseFormatException)

    @test
    def ReturnNonStrInTupleViaDummyModule(self):
        self.manifest = {
            u'component': {
                # u'param': u'@@(~DUMMY://return int in tuple)'
                u'param': u"{}{}{}{}return int in tuple{}".format(ygetparam.LEX_VAR_OPEN,
                                                                  ygetparam.LEX_MODULE_OPEN,
                                                                  self.module,
                                                                  ygetparam.LEX_MODULE_CLOSE,
                                                                  ygetparam.LEX_VAR_CLOSE)
            }
        }
        AssertRaises(lambda: ygetparam.GetParam(self.manifest, "component.param"),
                     ygetparam.ExtModResponseFormatException)


@suite
class SpecialVariables(TestSuite):

    def SetupSuite(self):
        super(SpecialVariables, self).SetupSuite()
        self.service = "service"
        self.manifest = {
            u'component': {
                u'param': u'@@(^self)'
            }
        }
        ygetparam.specVarSelf = self.service

    @test
    def SpecialVariableSELF(self):
        AssertEqual(ygetparam.GetParam(self.manifest, "component.param"), self.service)
