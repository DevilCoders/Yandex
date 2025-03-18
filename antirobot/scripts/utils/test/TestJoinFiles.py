import datetime

from devtools.fleur.ytest import suite, test, TestSuite
from devtools.fleur.ytest import AssertEqual, AssertTrue, AssertFalse

from antirobot.scripts.utils import join_files

def MakeFile1(f1Name):
    with open(f1Name, 'w') as f:
        print >>f, '1\tf1\ta'
        print >>f, '2\tf1\tb'
        print >>f, '3\tf1\tc'
        print >>f, '4\tf1\td'

def MakeFile2(f2Name):
    with open(f2Name, 'w') as f:
        print >>f, '2\tf2\t-\tb'
        print >>f, '4\tf2\t-\td'


@suite(package="antirobot.scripts.utils")
class JoinFiles(TestSuite):
    @test
    def SimpleJoin(self):
        f1Name = self.GetWorkPath('file1')
        f2Name = self.GetWorkPath('file2')
        fRes = self.GetWorkPath('result')

        MakeFile1(f1Name)
        MakeFile2(f2Name)

        join_files.SimpleJoinFiles(f1Name, f2Name, fRes)

        f = open(fRes)
        mergedLine = f.next()
        AssertEqual('2\tf1\tb\tf2\t-\tb', mergedLine.strip())

        mergedLine = f.next()
        AssertEqual('4\tf1\td\tf2\t-\td', mergedLine.strip())

        try:
            mergedLine = None
            mergedLine = f.next()
        except StopIteration:
            # We should get StopIteration exception
            mergedLine = ''

        AssertTrue(mergedLine is not None)
        AssertEqual('', mergedLine)

    @test
    def ComplexJoin(self):
        f1Name = self.GetWorkPath('file1')
        f2Name = self.GetWorkPath('file2')
        fRes = self.GetWorkPath('result')

        MakeFile1(f1Name)
        MakeFile2(f2Name)

        def KeyGetter1(line):
            return line.split('\t')[2]

        def KeyGetter2(line):
            return line.split('\t')[3]

        def MergeFunc(line1, line2):
            return '%s\t%s' % (line1, line2)

        join_files.JoinFiles(f1Name, f2Name, KeyGetter1, KeyGetter2, fRes, MergeFunc)

        f = open(fRes)
        mergedLine = f.next()
        AssertEqual('2\tf1\tb\t2\tf2\t-\tb', mergedLine.strip())

        mergedLine = f.next()
        AssertEqual('4\tf1\td\t4\tf2\t-\td', mergedLine.strip())

        try:
            mergedLine = None
            mergedLine = f.next()
        except StopIteration:
            # We should get StopIteration exception
            mergedLine = ''

        AssertTrue(mergedLine is not None)
        AssertEqual('', mergedLine)
