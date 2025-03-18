#include <library/cpp/monlib/encode/json/json.h>
#include <library/cpp/monlib/encode/spack/spack_v1.h>
#include <library/cpp/safe_stats/safe_stats.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/thread/pool.h>

using namespace NSFStats;

Y_UNIT_TEST_SUITE(SfStats) {

    static TString FormatStats(TStats& s) {
        TStringStream b;
        s.Out(TSolomonOut(b));
        return b.Str();
    }

    static TString FormatStatsSpack(TStats& s, TInstant ts) {
        TStringStream spackOut;
        s.Out(TSolomonOutSpack(spackOut, ts));

        TStringStream jsonOut;
        auto jsonEncoder = NMonitoring::EncoderJson(&jsonOut);
        NMonitoring::DecodeSpackV1(&spackOut, jsonEncoder.Get());

        return jsonOut.Str();
    }

    static TString JoinAns(TVector<TString> parts) {
        return TStringBuilder{}
            << R"D({"sensors":[)D"
            << JoinSeq(",", parts)
            << R"D(]})D";
    }

    class TMyMetric : public TBasicMetric {
    public:
        using TBasicMetric::TBasicMetric;
        using TBasicProxy = TStats::TContext::TBasicProxy<TMyMetric>;

        template <class TBase>
        class TProxy : public TBase {
        public:
            using TBase::TBase;

            void Inc(ui64 x) {
                this->AddFunc([x](auto* m) {
                    m->Inc(x);
                });
            }
            void Dec(ui64 x) {
                this->AddFunc([x](auto* m) {
                    m->Dec(x);
                });
            }
        };

        void Inc(ui64 x) noexcept {
            X_ += x * x;
        }
        void Dec(ui64 x) noexcept {
            X_ -= x * x;
        }
        void Visit(TFunc&& func) noexcept override {
            func(this->Name() + "_my", X_, true);
        }
    private:
        ui64 X_ = 0;
    };

    Y_UNIT_TEST(SimpleUint) {
        TStats s;
        {
            TStats::TContext ctx(s);
            ctx.Inc("some_value", 100);
        }

        TStringStream b;
        s.Out(TSolomonOut(b));
        TString answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"some_value\"},\"mode\":\"deriv\",\"value\":100}]}";
        UNIT_ASSERT_VALUES_EQUAL(b.Str(), answer);

        TStringStream b_retry;
        s.Out(TSolomonOut(b_retry));
        UNIT_ASSERT_VALUES_EQUAL(b_retry.Str(), answer);  // answer stays - metrics are derived
    }

    Y_UNIT_TEST(SimpleFloat) {
        TStats s;
        {
            TStats::TContext ctx(s);
            ctx.Inc("some_value", 423.54);
        }
        TString answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"some_value\"},\"mode\":\"deriv\",\"value\":423.54}]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(CustomMetric) {
        TStats s;
        {
            TStats::TContext ctx(s);
            ctx.Get<TMyMetric>("some_value").Inc(13);
            ctx.Get<TMyMetric>("some_value").Dec(3);
        }
        TString answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"some_value_my\"},\"mode\":\"deriv\",\"value\":160}]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(Percentile) {
        TStats s;
        {
            TStats::TContext ctx(s);
            using TPercenile = TPercentileMetric<6000, ui64, 1000, 0, 1, 500, 999, 1000>;
            auto m = ctx.Get<TPercenile>("some_value");
            for (ui64 i = 1; i < 5001; ++i) {
                m.Add(i);
            }
        }
        TString answer = TString() + "{\"sensors\":[" +
            "{\"labels\":{\"sensor\":\"some_value_p_0\"},\"value\":1},{\"labels\":{\"sensor\":\"some_value_p_0.1\"},\"value\":5}," +
            "{\"labels\":{\"sensor\":\"some_value_p_50\"},\"value\":2500},{\"labels\":{\"sensor\":\"some_value_p_99.9\"},\"value\":4995}," +
            "{\"labels\":{\"sensor\":\"some_value_p_100\"},\"value\":5000}" +
        "]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(Last) {
        TStats s;
        TStringStream b;
        {
            TStats::TContext ctx(s);
            auto m = ctx.Get<TLastMetric<double>>("last_double");
            m.Reset();
        }
        s.Out(TSolomonOut(b));
        TString zero_answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"last_double\"},\"value\":0}]}";
        UNIT_ASSERT_VALUES_EQUAL(b.Str(), zero_answer);

        {
            TStats::TContext ctx(s);
            auto m = ctx.Get<TLastMetric<double>>("last_double");
            m.Set(128.25);
            m.Set(256.5);
        }
        s.Out(TSolomonOut(b));
        TString answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"last_double\"},\"value\":256.5}]}";
        UNIT_ASSERT_VALUES_EQUAL(b.Str(), zero_answer + answer);

        {
            TStats::TContext ctx(s);
            auto m = ctx.Get<TLastMetric<double>>("last_double");
            m.Set(128.25);
            m.Reset();
        }
        s.Out(TSolomonOut(b));
        UNIT_ASSERT_VALUES_EQUAL(b.Str(), zero_answer + answer + zero_answer);
    }

    struct TTestInstant {
        static TInstant Value;

        static TInstant Now() {
            return Value;
        }
    };

    TInstant TTestInstant::Value;

    Y_UNIT_TEST(ExponentalMovingAverage) {
        TTestInstant::Value = TInstant::Zero();
        TStats s;

        constexpr ui64 MinDeltaUs = 5 * 1000000;
        constexpr ui32 Alpha = 15625;
        using TMetric = TExpMovingAverage<double, Alpha, MinDeltaUs, TTestInstant>;

        {
            auto context = TStats::TContext(s);
            auto metric = context.Get<TMetric>("test");
            metric.Add(1000000);
        }

        const auto Check = [&](ui64 timeUs, double value) {
            TTestInstant::Value = TInstant::MicroSeconds(timeUs);
            TString answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"test\"},\"value\":" + ToString(value) + "}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        };

        Check(0,                0);
        Check(MinDeltaUs / 2,   0);
        Check(MinDeltaUs * 2,   1562.5);
        Check(MinDeltaUs * 1.5, 1562.5);
        Check(MinDeltaUs * 4,   1538.0859375);
    }

    Y_UNIT_TEST(FollowAtomic) {
        TStats s;

        auto value = std::make_shared<std::atomic<ui64>>(42);
        {
            TStats::TContext ctx(s);
            ctx.Get<TAtomicMetric<ui64>>("last_atomic_ui64").Follow(value);
        }
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s),
            "{\"sensors\":[{\"labels\":{\"sensor\":\"last_atomic_ui64\"},\"value\":42}]}");

        *value = 123;
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s),
            "{\"sensors\":[{\"labels\":{\"sensor\":\"last_atomic_ui64\"},\"value\":123}]}");

        auto value2 = std::make_shared<std::atomic<ui64>>(1000);
        {
            TStats::TContext ctx(s);
            ctx.Get<TAtomicMetric<ui64>>("last_atomic_ui64").Follow(value2);
        }
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s),
            "{\"sensors\":[{\"labels\":{\"sensor\":\"last_atomic_ui64\"},\"value\":1123}]}");

        value.reset();
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s),
            "{\"sensors\":[{\"labels\":{\"sensor\":\"last_atomic_ui64\"},\"value\":1000}]}");

        value2.reset();
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s),
            "{\"sensors\":[{\"labels\":{\"sensor\":\"last_atomic_ui64\"},\"value\":0}]}");
    }

    Y_UNIT_TEST(SolomonPrefixSimple) {
        TStats s;
        {
            TStats::TContext ctx(s);
            TSolomonContext sctx(ctx, {}, "prefix");
            sctx.Inc("metric", 5);
        }
        TString answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"prefix.metric\"},\"mode\":\"deriv\",\"value\":5}]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(SolomonPrefixMultiple) {
        TStats s;
        {
            TStats::TContext ctx(s);
            TSolomonContext sctx(ctx, {}, "prefix1");
            TSolomonContext psctx(sctx, {}, "prefix2");
            psctx.Inc("metric", 5);
        }
        TString answer = "{\"sensors\":[{\"labels\":{\"sensor\":\"prefix1.prefix2.metric\"},\"mode\":\"deriv\",\"value\":5}]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(SolomonPrefixMultiContext) {
        {
            TStats s;
            {
                TSolomonContext ctxA(s, {}, "pref1");
                TSolomonContext ctxB(s, {}, "pref2");
                TSolomonMultiContext multiCtx{ctxA, ctxB};
                multiCtx.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"sensor\":\"pref1.x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"sensor\":\"pref2.x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
        {
            TStats s;
            {
                TSolomonContext ctx(s, {});
                TSolomonMultiContext multiCtx(ctx, TSolomonContext(ctx, {}, "pref"));
                multiCtx.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"sensor\":\"pref.x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
        {
            TStats s;
            {
                TSolomonContext ctxA(s, {}, "pref1");
                TSolomonContext ctxB(s, {}, "pref2");
                TSolomonContext ctxC(ctxB, {}, "pref3");
                TSolomonMultiContext multiCtx{ctxA, ctxC};
                multiCtx.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"sensor\":\"pref1.x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"sensor\":\"pref2.pref3.x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
        {
            TStats s;
            {
                TSolomonContext ctx(s, {});
                TSolomonMultiContext multiCtx(ctx, TSolomonContext(ctx, {}, "pref1"));
                TSolomonMultiContext multiCtxA(multiCtx, TSolomonContext(s, {}, "pref2"));
                multiCtxA.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"sensor\":\"pref1.x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"sensor\":\"pref2.x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
    }

    Y_UNIT_TEST(SolomonLabelsSimple) {
       TStats s;
        {
            TStats::TContext ctx(s);
            TSolomonContext sctx(ctx, {{"type", "1"}});
            sctx.Inc("metric", 5);
        }
        TString answer = "{\"sensors\":[{\"labels\":{\"type\":\"1\",\"sensor\":\"metric\"},\"mode\":\"deriv\",\"value\":5}]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(SolomonLabelsComplex) {
        TStats s;
        {
            TStats::TContext ctx(s);
            TSolomonContext sctx(ctx, {{"type", "1"}, {"abc", "def"}});
            {
                TSolomonContext(sctx, {{"other_type", "2"}, {"def", "abc"}}).Inc("metric", 5);
            }
        }
        TString answer = "{\"sensors\":[{\"labels\":{\"abc\":\"def\",\"def\":\"abc\",\"other_type\":\"2\",\"type\":\"1\",\"sensor\":\"metric\"},\"mode\":\"deriv\",\"value\":5}]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(SolomonNewContexts) {
        TStats s;
        TString sensor1 = R"D({"labels":{"other_type":"1","type":"1","sensor":"metric"},"mode":"deriv","value":10})D";
        TString sensor2 = R"D({"labels":{"other_type":"2","type":"1","sensor":"metric"},"mode":"deriv","value":20})D";
        TString sensor3 = R"D({"labels":{"other_type":"3","type":"1","sensor":"metric"},"mode":"deriv","value":30})D";
        {
            TSolomonContext ctx(s, {{"type", "1"}});
            {
                TSolomonContext(ctx, {{"other_type", "1"}}).Detached().Inc("metric", 10); // creates new context and updates TStats faster
                TSolomonContext(ctx.Detached(), {{"other_type", "2"}}).Inc("metric", 20); // creates new context and updates TStats faster
                TSolomonContext(ctx, {{"other_type", "3"}}).Inc("metric", 30);

                UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), JoinAns({sensor1, sensor2}));
            }
        }
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), JoinAns({sensor1, sensor2, sensor3}));
    }

    Y_UNIT_TEST(SolomonLabelsComplexWithOwningSolomonContext) {
        TStats s;
        {
            TSolomonContext sctx(s, {{"type", "1"}, {"abc", "def"}});
            auto sctx2 = sctx;
            {
                TSolomonContext(sctx2, {{"other_type", "2"}, {"def", "abc"}}).Inc("metric", 5);
            }
        }
        TString answer = "{\"sensors\":[{\"labels\":{\"abc\":\"def\",\"def\":\"abc\",\"other_type\":\"2\",\"type\":\"1\",\"sensor\":\"metric\"},\"mode\":\"deriv\",\"value\":5}]}";
        UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
    }

    Y_UNIT_TEST(IncAll) {
        TStats s;
        {
            TStats::TContext ctx(s);
            {
                THashMap<TString, ui64> metrics;
                metrics["s1"] += 100;
                metrics["s2"] += 200;
                ctx.IncAll(metrics);
            }
            {
                THashMap<TString, double> metrics;
                metrics["pi"] += 3.14;
                metrics["e"] += 2.72;
                ctx.IncAll(metrics);
            }
        }

        struct {
            void operator()(TStringBuf name, double value) {
                UNIT_ASSERT(name != "pi" || fabs(value - 3.14) < 1e-5);
                UNIT_ASSERT(name != "e" || fabs(value - 2.72) < 1e-5);
                ++Count;
            };

            void operator()(TStringBuf name, ui64 value) {
                UNIT_ASSERT(name != "s1" || value == 100);
                UNIT_ASSERT(name != "s2" || value == 200);
                ++Count;
            };

            int Count = 0;
        } checker;

        s.Out(checker);
        UNIT_ASSERT_VALUES_EQUAL(checker.Count, 4);
    }

    Y_UNIT_TEST(MultiContext) {
        {
            TStats s;
            {
                TSolomonContext ctxA(s, {{"type", "1"}, {"abc", "def"}});
                TSolomonContext ctxB(s, {{"type", "2"}, {"abc", "ghi"}});
                TSolomonMultiContext multiCtx{ctxA, ctxB};
                multiCtx.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"abc\":\"def\",\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"abc\":\"ghi\",\"type\":\"2\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
        {
            TStats s;
            {
                TSolomonContext ctx(s, {{"type", "1"}});
                TSolomonMultiContext multiCtx(ctx, TSolomonContext(ctx, {{"abc", "ghi"}}));
                multiCtx.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"abc\":\"ghi\",\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
        {
            TStats s;
            {
                TSolomonContext ctx(s, {{"type", "1"}});
                TSolomonMultiContext multiCtx(ctx, TSolomonContext(ctx, {{"abc", "ghi"}}));
                TSolomonMultiContext multiCtxA(multiCtx, TSolomonContext(s, {{"y", "x"}}));
                multiCtxA.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"abc\":\"ghi\",\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"y\":\"x\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
        {
            TStats s;
            {
                TSolomonContext ctx(s, {{"type", "1"}});
                TSolomonMultiContext multiCtx(ctx, TSolomonContext(ctx, {{"abc", "ghi"}}));
                TSolomonMultiContext multiCtx2(multiCtx, {{{"request", "another"}}});
                multiCtx2.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"abc\":\"ghi\",\"request\":\"another\",\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"request\":\"another\",\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
    }

    Y_UNIT_TEST(MultiContextDetached) {
        {
            TStats s;
            {
                TSolomonContext ctxA(s, {{"type", "1"}, {"abc", "def"}});
                TSolomonContext ctxB(s, {{"type", "2"}, {"abc", "ghi"}});
                TSolomonMultiContext multiCtx{ctxA, ctxB};
                auto detachedMultiCts = multiCtx.Detached();
                detachedMultiCts.Inc("x", 1);
            }
            TString answer = "{\"sensors\":["
                "{\"labels\":{\"abc\":\"def\",\"type\":\"1\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1},"
                "{\"labels\":{\"abc\":\"ghi\",\"type\":\"2\",\"sensor\":\"x\"},\"mode\":\"deriv\",\"value\":1}]}";
            UNIT_ASSERT_VALUES_EQUAL(FormatStats(s), answer);
        }
    }

    Y_UNIT_TEST(LongLivingContext) {
        THolder<TSolomonContext> ctx;
        {
            TStats s;
            ctx = MakeHolder<TSolomonContext>(
                TSolomonContext(s, {{"abc", "def"}})
            );
        }
        ctx->Inc("x", 1);
        ctx.Reset(); // expect no crash here
    }

    Y_UNIT_TEST(ThreadSafety) {
#ifndef _WINDOWS
        TStats s;
        auto ctx = TSolomonContext(
            TSolomonContext(s, {{"abc", "def"}})
        );
        ctx.Inc("x", 1);

        auto queue = CreateThreadPool(4);
        for (ui64 i = 0; i < 500000; ++i) {
            queue->SafeAddFunc([ctx]() mutable {
                ctx.Inc("x", 1);
                TSolomonContext(ctx, {{"label", "val"}}).Inc("y", 1);
                ctx.Detached().Inc("z", 1);
            });
        }
        queue->Stop();
#endif
    }

    Y_UNIT_TEST(SpackRate) {
        const TInstant ts = TInstant::Now();
        TStats s;

        {
            TStats::TContext ctx(s);
            ctx.Inc("value_1_int", 100);
        }
        TStringStream answer;
        answer << R"({"sensors":[{"kind":"RATE","labels":{"sensor":"value_1_int"})"
            << R"(,"value":100}]})";
        UNIT_ASSERT_VALUES_EQUAL(FormatStatsSpack(s, ts), answer.Str());

        {
            TStats::TContext ctx(s);
            ctx.Inc("value_2_double", 123.456);
        }
        TStringStream answer_2_vals;
        answer_2_vals << R"({"sensors":[{"kind":"RATE","labels":{"sensor":"value_2_double"})"
            << R"(,"value":123},{"kind":"RATE","labels":{"sensor":"value_1_int"})"
            << R"(,"value":100}]})";
        UNIT_ASSERT_VALUES_EQUAL(FormatStatsSpack(s, ts), answer_2_vals.Str());
    }

    Y_UNIT_TEST(SpackGauge) {
        const TInstant ts = TInstant::Now();
        TStats s;

        {
            TStats::TContext ctx(s);
            auto m = ctx.Get<TLastMetric<double>>("last_double");
            m.Set(128.25);
            m.Set(256.5);
        }
        TStringStream answer_last_double;
        answer_last_double << R"({"sensors":[{"kind":"GAUGE","labels":{"sensor":"last_double"},"ts":)"
            << ts.Seconds() << R"(,"value":256.5}]})";
        UNIT_ASSERT_VALUES_EQUAL(FormatStatsSpack(s, ts), answer_last_double.Str());

        {
            TStats::TContext ctx(s);
            auto m = ctx.Get<TLastMetric<ui64>>("last_int");
            m.Set(1234);
            m.Set(123);
        }
        TStringStream answer_last_int;
        answer_last_int << R"({"sensors":[{"kind":"GAUGE","labels":{"sensor":"last_double"},"ts":)"
            << ts.Seconds() << R"(,"value":256.5},{"kind":"GAUGE","labels":{"sensor":"last_int"},"ts":)"
            << ts.Seconds() << R"(,"value":123}]})";
        UNIT_ASSERT_VALUES_EQUAL(FormatStatsSpack(s, ts), answer_last_int.Str());
    }
}

Y_UNIT_TEST_SUITE(DictStats) {
    Y_UNIT_TEST(Simple) {
        NSFStats::TDictStats s;
        {
            TStats::TContext ctx = s["key"];
            ctx.Inc("val", (ui64)100);
        }
        {
            TStats::TContext ctx = s["other_key"];
            ctx.Inc("val", (ui64)123);
        }

        TStringStream b;

        auto func = [&b](const TString& dictKey, const TString& valKey, ui64 value) {
            b << dictKey << "." << valKey << "=" << value << Endl;
        };
        s.Out(func);

        TString expected = "key.val=100\nother_key.val=123\n";
        UNIT_ASSERT_VALUES_EQUAL(b.Str(), expected);
    }
}
