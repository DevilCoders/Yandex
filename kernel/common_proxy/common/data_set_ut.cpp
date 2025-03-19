#include <library/cpp/testing/unittest/registar.h>
#include <kernel/common_proxy/common/data_set.h>

Y_UNIT_TEST_SUITE(CProxyDataSetSuite) {

    class TTestObject : public NCommonProxy::TObject {
    public:
        TTestObject(int value = 0)
            : Value(value)
        {}

        int Value;
    };

    class TTestObjectDerived : public TTestObject {
    public:
        TTestObjectDerived(int value)
            : TTestObject(value)
        {}

        bool Derived = true;
    };

    struct TTestData : public TAtomicRefCount<TTestData> {
        TTestData(int value = 0)
            : Value(value)
        {}

        int Value;
    };

    typedef NCommonProxy::TObjectHolder<TTestData> TTestDataHolder;

    class TMeta : public NCommonProxy::TMetaData {
    public:
        TMeta() {
            Register("blob", TMetaData::dtBLOB);
            Register("bool", TMetaData::dtBOOL);
            Register("double", TMetaData::dtDOUBLE);
            Register("int", TMetaData::dtINT);
            Register("string", TMetaData::dtSTRING);
            RegisterObject<TTestObject>("object");
            RegisterObject<TTestObject>("derived");
            RegisterObject<TTestObjectDerived>("derived2");
            RegisterObject<TTestDataHolder>("holder1");
            RegisterObject<TTestDataHolder>("holder2");
            RegisterPointer<TTestData>("ptr");
            RegisterPointer<const TTestData>("const_ptr");
        }
    };

    Y_UNIT_TEST(SetAndGet) {
        TString s("abc");
        double d(1.2345);
        int i(12345);
        bool b(true);
        TTestData t(i);
        auto object = MakeIntrusive<TTestObject>(3 * i);
        TTestData* ptr = &t;
        const TTestData* constPtr = &t;

        const TMeta meta;
        NCommonProxy::TDataSet::TPtr data = MakeIntrusive<NCommonProxy::TDataSet>(meta);
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->Set<TBlob>("blob", TBlob::FromString(s));
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->Set<bool>("bool", b);
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->Set<double>("double", d);
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->Set<i64>("int", i);
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->Set<TString>("string", s);
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->SetObject<TTestObject>("object", object);
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->SetObject<TTestObject>("derived", MakeIntrusive<TTestObjectDerived>(6 * i));
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->SetObject<TTestObject>("derived2", MakeIntrusive<TTestObjectDerived>(8 * i));
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->SetObject<TTestDataHolder>("holder1", MakeIntrusive<TTestDataHolder>(t));
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->SetObject<TTestDataHolder>("holder2", MakeIntrusive<TTestDataHolder>(i * 2));
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->SetPointer<TTestData>("ptr", ptr);
        UNIT_ASSERT_C(!data->AllFieldsSet(), "Not all fields set, but data->AllFieldsSet()");

        data->SetPointer<const TTestData>("const_ptr", constPtr);
        UNIT_ASSERT_C(data->AllFieldsSet(), "All fields set, but !data->AllFieldsSet()");

        UNIT_ASSERT_EQUAL(data->Get<TBlob>("blob").AsCharPtr(), s);
        UNIT_ASSERT_EQUAL(data->Get<bool>("bool"), b);
        UNIT_ASSERT_EQUAL(data->Get<double>("double"), d);
        UNIT_ASSERT_EQUAL(data->Get<i64>("int"), i);
        UNIT_ASSERT_EQUAL(data->Get<TString>("string"), s);

        auto getedObject = data->GetObject<TTestObject>("object");
        UNIT_ASSERT(getedObject);
        UNIT_ASSERT_EQUAL(getedObject.Get(), object.Get());
        UNIT_ASSERT_EQUAL_C(getedObject->Value, i * 3, "object changed");

        auto derived = data->GetObject<TTestObjectDerived>("derived");
        UNIT_ASSERT(derived);
        UNIT_ASSERT_EQUAL_C(derived->Value, i * 6, "derived changed");

        UNIT_ASSERT_EQUAL((*data->GetObject<TTestDataHolder>("holder1"))->Value, t.Value);
        UNIT_ASSERT_EQUAL((*data->GetObject<TTestDataHolder>("holder2"))->Value, 2 * i);
        UNIT_ASSERT_EQUAL(data->GetPointer<TTestData>("ptr"), ptr);
        UNIT_ASSERT_EQUAL(data->GetPointer<const TTestData>("const_ptr"), constPtr);

        UNIT_ASSERT_EXCEPTION(data->GetObject<TTestObjectDerived>("object"), yexception);
        UNIT_ASSERT_EXCEPTION(data->SetObject<TTestObject>("derived2", object), yexception);
        UNIT_ASSERT_EXCEPTION(data->Set<i64>("double", i), yexception);
    }
}
