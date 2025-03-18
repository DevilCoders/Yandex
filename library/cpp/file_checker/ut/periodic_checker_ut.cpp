#include <library/cpp/file_checker/file_checker.h>

#include <library/cpp/testing/unittest/registar.h>

#include <stdexcept>

namespace NChecker {
    struct TListenerMock: public IUpdater {
        TListenerMock()
            : Checked()
            , Changed()
            , Reacted()
        {
        }

        void Change(bool ch) {
            Checked = false;
            Reacted = false;
            Changed = ch;
        }

        bool Update() override {
            UNIT_ASSERT(Changed);
            UNIT_ASSERT(!Reacted);
            Reacted = true;
            Changed = false;
            return true;
        }

        bool OnCheck() override {
            Checked = true;
            return true;
        }

        bool IsChecked() const {
            return Checked;
        }

        bool IsChanged() const override {
            return Changed;
        }

        bool IsReacted() const {
            return Reacted;
        }

        void Check(NChecker::TPeriodicChecker& check, TSystemEvent& ev, bool ch, int line) {
            check.Wake();

            ui32 n = 1000;
            while (--n && !IsChecked())
                ev.Wait(5);

            UNIT_ASSERT_C(IsChecked(), line); // if no reaction in 5 seconds consider failure

            if (ch) {
                UNIT_ASSERT_C(IsReacted(), line);
            } else {
                UNIT_ASSERT_C(!IsReacted(), line);
            }
        }

    private:
        bool Checked;
        bool Changed;
        bool Reacted;
    };

    struct TMockAction {
        TDuration Lag;
        mutable bool Done;

        TMockAction(TDuration sleep)
            : Lag(sleep)
            , Done()
        {
        }

        void Do() const {
            if (Lag.MicroSeconds())
                Sleep(Lag);
            Done = true;
        }
    };

}

class TPeriodicCheckerTest: public TTestBase {
    UNIT_TEST_SUITE(TPeriodicCheckerTest);
    UNIT_TEST(TestPeriodicCheck)
    UNIT_TEST_SUITE_END();

private:
    void TestPeriodicCheck() {
        using namespace NChecker;

        TSystemEvent ev(TSystemEvent::rAuto);
        {
            TListenerMock mock;
            TPeriodicChecker checker(mock, -1);

            checker.Start();
            mock.Check(checker, ev, false, __LINE__);
            mock.Check(checker, ev, false, __LINE__);

            mock.Change(true);
            mock.Check(checker, ev, true, __LINE__);
            mock.Check(checker, ev, true, __LINE__);

            mock.Change(false);
            mock.Check(checker, ev, false, __LINE__);
            mock.Check(checker, ev, false, __LINE__);

            mock.Change(true);
            mock.Check(checker, ev, true, __LINE__);
            mock.Check(checker, ev, true, __LINE__);

            mock.Change(false);
            mock.Check(checker, ev, false, __LINE__);
            mock.Check(checker, ev, false, __LINE__);
        }
        ev.Signal();
    }
};

UNIT_TEST_SUITE_REGISTRATION(TPeriodicCheckerTest);
