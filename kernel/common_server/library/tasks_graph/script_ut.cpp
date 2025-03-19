#include "script.h"

#include <util/string/vector.h>
#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/unittest/gtest.h>

namespace {
    TMutex Mutex;
}

class TTask: public NRTYScript::ITask {
private:
    ui32 CountToOk;
    mutable ui32 CountRun;
    TString Name;
    TStringStream* Out;
    TString Link;
    mutable ui32 Info;
    ui32 LinkedInfo;
public:

    static TFactory::TRegistrator<TTask> Registrator;

    TTask() {
        LinkedInfo = 0;
        Info = 0;
        Out = nullptr;
    }

    virtual void TakeInfo(const NRTYScript::ITasksInfo& info) {
        if (!!Link) {
            NJson::TJsonValue result;
            info.GetValueByPath(Link, result);
            LinkedInfo = result.GetIntegerRobust();
        }
    }

    TTask(TStringStream& out, const TString& name, ui32 countToOk = 1)
        : Out(&out)
    {
        LinkedInfo = 0;
        Info = 0;
        CountToOk = countToOk;
        CountRun = 0;
        Name = name;
    }

    TTask(TStringStream& out, const TString& name, const TString& link, ui32 countToOk = 1)
        : Out(&out)
    {
        LinkedInfo = 0;
        Info = 0;
        Link = link;
        CountToOk = countToOk;
        CountRun = 0;
        Name = name;
    }

    virtual void Fail(const TString&) {

    }

    virtual NJson::TJsonValue Serialize() const {
        NJson::TJsonValue result(NJson::JSON_MAP);
        result["name"] = Name;
        result["info"] = Info;
        result["linked_info"] = LinkedInfo;
        result["count_ok"] = CountToOk;
        result["count_run"] = CountRun;
        return result;
    }

    virtual void Deserialize(const NJson::TJsonValue& info) {
        Name = info["name"].GetStringRobust();
        CountToOk = info["count_ok"].GetInteger();
        CountRun = info["count_run"].GetInteger();
        LinkedInfo = info["linked_info"].GetInteger();
        Info = info["info"].GetInteger();
    }

    virtual TString GetName() const {
        return Name;
    }

    virtual TString GetClass() const {
        return "test_task";
    }

    virtual bool Execute(void* externalInfo) const {
        Info = LinkedInfo + 1;
        Sleep(TDuration::MilliSeconds(10 + rand() % 10));
        TGuard<TMutex> g(::Mutex);
        ++CountRun;
        if (Out) {
            *Out << GetName() << " ";
        } else {
            (*(TStringStream*)externalInfo) << GetName() << " ";
        }
        return CountRun >= CountToOk;
    }

    virtual TString GetInformation() const {
        return ToString(CountToOk) + "/" + ToString(CountRun);
    }
};

TTask::TFactory::TRegistrator<TTask> TTask::Registrator("test_task");

Y_UNIT_TEST_SUITE(TRtyScriptSuite) {
    Y_UNIT_TEST(TestSimpleLinear) {
        TStringStream ss;
        NRTYScript::TScript script;
        TString result;
        NRTYScript::TTaskContainer predInfo;
        for (ui32 i = 0; i < 100; ++i) {
            NRTYScript::TTaskContainer currInfo = script.AddTaskLinear(new TTask(ss, ToString(i), 1));
            result += ToString(i) + " ";
            if (i > 0) {
                script.AddSeqInfo(predInfo.GetName(), currInfo.GetName(), new NRTYScript::TFinishedChecker());
            }
            predInfo = currInfo;
        }

        script.Execute(16);

        ASSERT_EQ(ss.Str(), result);
        return;
    }

    Y_UNIT_TEST(TestLinear) {
        TStringStream ss;
        NRTYScript::TScript script;
        TString result;
        TString predName;
        for (ui32 i = 0; i < 100; ++i) {
            predName = script.AddTaskLinear(new TTask(ss, ToString(i), 1)).GetName();
            result += ToString(i) + " ";
        }

        script.Execute(16);

        NJson::TJsonValue json;
        script.GetValueByPath(predName + "-info", json);
        ASSERT_EQ(json.GetInteger(), 1);

        ASSERT_EQ(ss.Str(), result);
        return;
    }

    Y_UNIT_TEST(TestLinearInfo) {
        TStringStream ss;
        NRTYScript::TScript script;
        TString result;
        TString predName;
        for (ui32 i = 0; i < 100; ++i) {
            if (!!predName)
                predName = script.AddTaskLinear(new TTask(ss, ToString(i), predName + "-info", 1)).GetName();
            else
                predName = script.AddTaskLinear(new TTask(ss, ToString(i), 1)).GetName();
            result += ToString(i) + " ";
        }

        script.Execute(16);

        NJson::TJsonValue json;
        script.GetValueByPath(predName + "-info", json);
        ASSERT_EQ(json.GetInteger(), 100);

        ASSERT_EQ(ss.Str(), result);
        return;
    }

    Y_UNIT_TEST(TestLinearConstructionGreat) {
        TStringStream ss;
        NRTYScript::TScript script;
        TString result;
        for (ui32 i = 0; i < 100; ++i) {
            script.AddTaskAfterAll(new TTask(ss, ToString(i), 1));
            result += ToString(i) + " ";
        }

        script.Execute(16);

        ASSERT_EQ(ss.Str(), result);
        return;
    }

    Y_UNIT_TEST(TestRandom) {
        TStringStream ss;
        NRTYScript::TScript script;
        TString result;
        for (ui32 i = 0; i < 100; ++i) {
            script.AddTask(new TTask(ss, ToString(i), 1));
            result += ToString(i) + " ";
        }

        script.Execute(16);

        ASSERT_NE(ss.Str(), result);
        return;
    }

    Y_UNIT_TEST(TestOnePlusRandom) {
        TStringStream ss;
        NRTYScript::TScript script;
        TString result;
        auto t1 = script.AddTask(new TTask(ss, "first", 1));
        result = "first ";
        for (ui32 i = 0; i < 100; ++i) {
            script.AddSeqInfo(t1, script.AddTask(new TTask(ss, ToString(i), 1)), new NRTYScript::TFinishedChecker());
            result += ToString(i) + " ";
        }


        script.Execute(16);

        TVector<TString> vs = SplitString(ss.Str(), " ");
        ASSERT_EQ(vs[0], "first");
        ASSERT_NE(ss.Str(), result);
        return;
    }

    Y_UNIT_TEST(TestRandomPlusOne) {
        TStringStream ss;
        NRTYScript::TScript script;
        TString result;
        for (ui32 i = 0; i < 100; ++i) {
            script.AddTask(new TTask(ss, ToString(i), 1));
            result += ToString(i) + " ";
        }
        script.AddTaskAfterAll(new TTask(ss, "last", 1));
        result += "last ";

        script.Execute(16);

        TVector<TString> vs = SplitString(ss.Str(), " ");
        ASSERT_EQ(vs.back(), "last");
        ASSERT_NE(ss.Str(), result);
        return;
    }

    Y_UNIT_TEST(TestTree) {
        TStringStream ss;
        NRTYScript::TScript script;
        const auto t1 = script.AddTask(new TTask(ss, "1", 1));
        const auto t2 = script.AddTask(new TTask(ss, "2", 1));
        const auto t3 = script.AddTask(new TTask(ss, "3", 1));
        const auto t4 = script.AddTask(new TTask(ss, "4", 1));
        script.AddSeqInfo(t1, t3, new NRTYScript::TFinishedChecker());
        script.AddSeqInfo(t2, t3, new NRTYScript::TFinishedChecker());
        script.AddSeqInfo(t3, t4, new NRTYScript::TFinishedChecker());

        script.Execute(16);

        ASSERT_TRUE(ss.Str() == "1 2 3 4 " || ss.Str() == "2 1 3 4 ")
        return;
    }

    Y_UNIT_TEST(TestRepeat) {
        TStringStream ss;
        NRTYScript::TScript script;
        auto t1 = script.AddTask(new TTask(ss, "1", 1));
        auto t2 = script.AddTask(new TTask(ss, "2", 1));
        auto t3 = script.AddTask(new TTask(ss, "3", 2));
        auto t4 = script.AddTask(new TTask(ss, "4", 2));
        script.AddSeqInfo(t1, t3, new NRTYScript::TSucceededChecker);

        script.AddSeqInfo(t2, t3, new NRTYScript::TSucceededChecker);

        script.AddSeqInfo(t3, t2, new NRTYScript::TFailedChecker);
        script.AddSeqInfo(t3, t1, new NRTYScript::TFailedChecker);
        script.AddSeqInfo(t3, t4, new NRTYScript::TSucceededChecker);

        script.Execute(16);

        Cerr << ss.Str() << Endl;
        TVector<TString> vs = SplitString(ss.Str(), " ");
        ASSERT_EQ(vs[2], "3");
        ASSERT_EQ(vs[5], "3");
        ASSERT_EQ(vs[6], "4");
        ASSERT_NE(vs[1], vs[0]);
        ASSERT_NE(vs[4], vs[3]);
        ASSERT_TRUE(vs[0] == "1" || vs[0] == "2");
        ASSERT_TRUE(vs[1] == "1" || vs[1] == "2");
        ASSERT_TRUE(vs[3] == "1" || vs[3] == "2");
        ASSERT_TRUE(vs[4] == "1" || vs[4] == "2");

        return;
    }

    Y_UNIT_TEST(TestSerialization) {
        TStringStream ss;
        NRTYScript::TScript script;
        auto t1 = script.AddTask(new TTask(ss, "1", 1));
        auto t2 = script.AddTask(new TTask(ss, "2", 1));
        auto t3 = script.AddTask(new TTask(ss, "3", 2));
        auto t4 = script.AddTask(new TTask(ss, "4", 2));
        script.AddSeqInfo(t1, t3, new NRTYScript::TSucceededChecker);

        script.AddSeqInfo(t2, t3, new NRTYScript::TSucceededChecker);

        script.AddSeqInfo(t3, t2, new NRTYScript::TFailedChecker);
        script.AddSeqInfo(t3, t1, new NRTYScript::TFailedChecker);
        script.AddSeqInfo(t3, t4, new NRTYScript::TSucceededChecker);


        NRTYScript::TScript scriptNew;
        scriptNew.Deserialize(script.Serialize());
        scriptNew.Execute(16, &ss);

        TVector<TString> vs = SplitString(ss.Str(), " ");
        ASSERT_EQ(vs[2], "3");
        ASSERT_EQ(vs[5], "3");
        ASSERT_EQ(vs[6], "4");
        ASSERT_NE(vs[1], vs[0]);
        ASSERT_NE(vs[4], vs[3]);
        ASSERT_TRUE(vs[0] == "1" || vs[0] == "2");
        ASSERT_TRUE(vs[1] == "1" || vs[1] == "2");
        ASSERT_TRUE(vs[3] == "1" || vs[3] == "2");
        ASSERT_TRUE(vs[4] == "1" || vs[4] == "2");

        return;
    }

    Y_UNIT_TEST(TestContinue) {
        TStringStream ss;
        NRTYScript::TScript script;
        auto t1 = script.AddTask(new TTask(ss, "1", 1));
        auto t2 = script.AddTask(new TTask(ss, "2", 1));
        auto t3 = script.AddTask(new TTask(ss, "3", 2));
        auto t4 = script.AddTask(new TTask(ss, "4", 1));
        script.AddSeqInfo(t1, t3, new NRTYScript::TSucceededChecker);

        script.AddSeqInfo(t2, t3, new NRTYScript::TSucceededChecker);

        script.AddSeqInfo(t3, t4, new NRTYScript::TSucceededChecker);

        script.Execute(16, &ss);

        TVector<TString> vs = SplitString(ss.Str(), " ");

        ASSERT_EQ(vs.size(), 3);
        ASSERT_NE(vs[1], vs[0]);
        ASSERT_TRUE(vs[0] == "1" || vs[0] == "2");
        ASSERT_TRUE(vs[1] == "1" || vs[1] == "2");
        ASSERT_EQ(vs[2], "3");

        NRTYScript::TScript scriptNew;
        scriptNew.Deserialize(script.Serialize());

        ss.clear();
        scriptNew.Execute(16, &ss);

        vs = SplitString(ss.Str(), " ");
        ASSERT_EQ(vs.size(), 0);

        return;
    }

}
