#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/string/printf.h>
#include <util/system/tempfile.h>
#include <util/system/thread.h>

#include "lcookie.h"
#include "lkey_reader.h"
#include "_atomic_copier.h"

const char* LCOOKIE = "SG0TWlVvB1J9DXZXW0AHWw9neXB1bncCEjgrJkYPUmdMHgIvKx1YAicOERR7XhghDQIPElM6dQc9JwtaXTsDHw==.1311989535.9065.287854.22faf2292da9c7dbabc30e8512cc098d";

const char* KEYS[] = {
    "9060;7gNFasAOAhJ5lile7R10YYNg09Kk3dzRCglVi0FzcdJpobkJdSSV5FKYTEBFvP6r;1311451201\n\
9059;ETVBXbx8fZIqWsFmWdLnEUNhLfrbac5QEGCVzb49aH7CoK33oUURaCwiO9OEqvey;1311364801\n\
9058;BCaNMgefNEeXRx40JqHgs86RFNYYPwyny6RxyJyGQezpd5WQUPGtD6KsA4SyB9gY;1311278401\n\
9057;qiixveLqqXQOLrDQAi5nHQRmcpVo5N81PEEH5UZ7IEBeM5Hyc7oAC76HVAkd7GJI;1311192001\n\
9056;5MAvfZvZFcYhT9UagdLXUM53aeMCoglO1NdWon5LmT3eR3OpKGzH40lt2qpkBe50;1311105601\n\
9055;16pBkVMYmnKMsnn9A8OGGUf8v0UHlvUdpN0b3tLXGiPK7fd9ND52k7iDEPlMuTSb;1311019201\n\
9054;mWC6JLCbnUnZ0Kg1cYptvhNVPnvVlJLnd0SPPmMbRjH9TQ75UQGvlVRC1rQMM83h;1310932801\n\
9053;LY2uxHsDsUXbhR2Qjbmrweldrav4hvtkSesaRbs7nVnAuRckqbmB0mLBzGSA6rYr;1310846401\n\
9052;qQLspqasz9VPerd7CyA8fdHhUkuNOjHHxXX4eJn8oD8CGp9uvg4kpF9PZNrHIvsu;1310760001\n\
9051;isdorNLJXamRfsFRmX7cQ6cWwLkRnUVzzIQZj5tAK5dfa86Mql8LfKoirft9WgyV;1310673601\n\
9050;yJLC8Ow0RHf09rn0OoKtHBerO9v4QqMkOkpQXH7GWIAOdMhKLTcI12FxgQem6biv;1310587201",

    "9070;O0KcZ4clquudu4zW47H6fk5kmc5A6QZ2a2SN4ecQndaGqQfU3hp2sfO4n8Tg6Dcs;1312315201\n\
9069;SyUiQTF6unDlYoTThnmRxf5NYOl2cuhJ9vEHSdibujxsq2C8xj0BWO4dejmwXpm3;1312228801\n\
9068;FEbPsmN1vGXUbKHkH4HIIvTT3ddcYIi6HkylWErd5G366X13tqL1w8mTd1CrO4GA;1312142401\n\
9067;CLjtBJhDeMaaEEPweQufs3EqS6TDaaKjeZU4W1AIva6jNeMC31fW5cir7OFHUmQu;1312056001\n\
9066;Z1kXJJww2vKuZHOZZsC5cYbpsMyfkywaBfwXw8P8i5TAJr4U2F0Vk9IFROVRKUJr;1311969601\n\
9065;pUymaX1jO5Aejv4j7VIBLXO6cRJI7j4UxqgBBr7pVkkbV7wGdiie2QAvQS8j7Hop;1311883201",

    "9064;Lorne3hchcusTGuZeEvF1Zp5OPvwefHFpkuLNKqs3FoCINNkNtyOMgJb4DpzGmMy;1311796801\n\
9063;9WdzFqYNYq6yl5p4Dst0dOsVUzEfBi0dHmLEbbRNauVFaCf2ZK12WMw6Ic8bRUeV;1311710401\n\
9062;IeYWqA84OFjba55T4gwI6NVnNTU2MoXaDRpfSmuSMj6jgBrBG2TZKAzxmTgiClNZ;1311624001\n\
9061;KrEhiFda3Yev1gExog6NhQixqRLlIjGZyucNcXWhfsx7nA5Ot5LQnsfggnFlOHwq;1311537601"};

using namespace NLCookie;

Y_UNIT_TEST_SUITE(TTestLCookie) {
    typedef TAtomicSharedPtr<IKeychain> TKeychainPtr;

    TKeychainPtr LoadKeychain(size_t first, size_t last) {
        TTempFile tf(MakeTempName(nullptr, "lcookie_keys"));

        {
            TFixedBufferFileOutput f(tf.Name());
            for (size_t i = first; i <= last; ++i) {
                f << KEYS[i];
            }
        }

        return TKeychainPtr(new TFileKeyReader(tf.Name()));
    }

    TKeychainPtr LoadKeychain(size_t index) {
        return LoadKeychain(index, index);
    }

    inline void VerifyCookie(TKeychainPtr kc) {
        auto maybe = TryParse(LCOOKIE, *kc);
        UNIT_ASSERT(maybe.Defined());
        UNIT_ASSERT_VALUES_EQUAL(maybe.GetRef().Login, "v-iofik");
    }

    Y_UNIT_TEST(TestParse) {
        TAtomicCopier<TKeychainPtr> kc(LoadKeychain(1));
        VerifyCookie(kc);
    }

    Y_UNIT_TEST(TestReload) {
        TAtomicCopier<TKeychainPtr> kc(LoadKeychain(0));
        UNIT_ASSERT(TryParse(LCOOKIE, *static_cast<TKeychainPtr>(kc)).Empty());

        for (size_t last = 1; last < Y_ARRAY_SIZE(KEYS); ++last) {
            kc = LoadKeychain(0, last);
            VerifyCookie(kc);
        }
    }

    struct TThrParams {
        TAtomicCopier<TKeychainPtr>* KC;
        TAtomic Cnt;
    };

    void* LCookieParserProc(void* p) {
        TThrParams* pp = (TThrParams*)p;
        TAtomicCopier<TKeychainPtr>& kc = *pp->KC;

        try {
            while (true) {
                AtomicIncrement(pp->Cnt);
                auto maybe = TryParse(LCOOKIE, *static_cast<TKeychainPtr>(kc));
                if (maybe.Defined()) {
                    UNIT_ASSERT_VALUES_EQUAL(maybe.GetRef().Login, "v-iofik");
                    return nullptr;
                }
            }
        } catch (const yexception& e) {
            return new TString(e.what());
        }

        return nullptr;
    }

    Y_UNIT_TEST(TestMultipleThreads) {
        typedef TVector<TAutoPtr<TThread>> TThreadVector;
        typedef THashMap<TString, unsigned> TErrorsCounter;

        const size_t n = 100;

        TAtomicCopier<TKeychainPtr> kc(LoadKeychain(0));

        TThrParams tp = {
            &kc,
            0};

        TThreadVector threads;

        for (size_t i = 0; i < n; ++i) {
            threads.push_back(new TThread(LCookieParserProc, &tp));
            threads.back()->Start();
        }

        while ((size_t)AtomicGet(tp.Cnt) < n * 100) {
            Sleep(TDuration::MilliSeconds(10));
        }

        kc = LoadKeychain(0, 1);

        TErrorsCounter errors;
        size_t errorsNumber = 0;
        for (const auto& thread : threads) {
            void* error = thread->Join();

            if (error) {
                THolder<TString> s((TString*)error);

                ++errorsNumber;
                ++errors[*s];
            }
        }

        if (errorsNumber != 0) {
            yexception e;
            e << Sprintf("%lu of %lu threads failed:", errorsNumber, threads.size()) << Endl;
            for (TErrorsCounter::const_iterator it = errors.begin(); it != errors.end(); ++it) {
                e << Sprintf("%u x ", it->second) << it->first << Endl;
            }
            ythrow e;
        }
    }
}
