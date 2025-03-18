#include "array2d.h"
#include "array2d_writer.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/tempfile.h>

Y_UNIT_TEST_SUITE(TFileMapped2DArrayTest) {
    static const char TempFileName[] = "./FileMapped2DArray-test";
    static const bool USE_MAP = true;
    static const bool NOT_USE_MAP = false;

    template <typename I, typename T>
    void Write(TFile2DArrayWriter<I, T> & writer) {
        // row 0
        writer.Write('a');
        writer.Write('b');
        writer.Write('c');
        // row 1 - empty
        writer.NewLine();
        // row 2
        writer.NewLine();
        writer.Write('d');
        // row 3 - last, empty
        writer.NewLine();
        writer.Finish();
    }

    template <typename I, typename T>
    void Read(FileMapped2DArray<I, T> & reader, bool useMap = NOT_USE_MAP) {
        if (useMap) {
            TMemoryMap mapping(TempFileName);
            mapping.Map(0, mapping.Length());
            UNIT_ASSERT(reader.Init(mapping));
        } else
            UNIT_ASSERT(reader.Init(TempFileName));

        UNIT_ASSERT_VALUES_EQUAL(reader.Size(), 4u);
        // normal row
        UNIT_ASSERT_EQUAL(reader[0], reader.GetBegin(0));
        UNIT_ASSERT_VALUES_EQUAL(reader.GetLength(0), 3u);
        UNIT_ASSERT_EQUAL(reader.GetBegin(0) + reader.GetLength(0), reader.GetEnd(0));
        UNIT_ASSERT_EQUAL(reader[0][1], 'b');
        UNIT_ASSERT_EQUAL(reader.GetAt(0, 1), 'b');
        // empty row
        UNIT_ASSERT_EQUAL(reader[1], reader.GetBegin(1));
        UNIT_ASSERT_VALUES_EQUAL(reader.GetLength(1), 0u);
        UNIT_ASSERT_EQUAL(reader.GetBegin(1) + reader.GetLength(1), reader.GetEnd(1));
        // last row
        UNIT_ASSERT_EQUAL(reader[3], reader.GetBegin(3));
        UNIT_ASSERT_VALUES_EQUAL(reader.GetLength(3), 0u);
        UNIT_ASSERT_EQUAL(reader.GetBegin(3) + reader.GetLength(3), reader.GetEnd(3));
    }

    Y_UNIT_TEST(TestWriteUI32) {
        TFile2DArrayWriter<ui32, char> writer(TempFileName);
        Write(writer);
    }

    Y_UNIT_TEST(TestReadUI32) {
        TTempFile tmpFile(TempFileName);
        FileMapped2DArray<ui32, char> reader;
        Read(reader);
        reader.Term();
    }

    struct TTestStruct {
        ui16 A = 0;
        ui16 Padding = 0; // SEARCH-2776
        ui32 B = 0;
        ui64 C = 0;

        TTestStruct() = default;

        TTestStruct(ui16 a, ui32 b, ui64 c) {
            A = a;
            B = b;
            C = c;
        }

        bool operator==(const TTestStruct& val) const {
            return (A == val.A) && (B == val.B) && (C == val.C);
        }
    };

    struct TTestStructSmall {
        ui16 A = 0;
        ui32 B = 0;

        TTestStructSmall() = default;

        TTestStructSmall(ui16 a, ui32 b) {
            A = a;
            B = b;
        }

        bool operator==(const TTestStructSmall& val) const {
            return (A == val.A) && (B == val.B);
        }
    };

    static_assert(sizeof(TTestStruct) == sizeof(ui64) * 2, "Q:Why? A:SEARCH-2776");

    struct TTestStructBig {
        ui16 A = 0;
        ui32 B = 0;
        ui64 C = 0;
        ui16 D = 0;

        TTestStructBig() = default;

        TTestStructBig(ui16 a, ui32 b, ui64 c) {
            A = a;
            B = b;
            C = c;
        }

        bool operator==(const TTestStructBig& val) const {
            return (A == val.A) && (B == val.B) && (C == val.C) && (D == val.D);
        }
    };

    Y_UNIT_TEST(TestUpdateStructureZeroRecords) {
        TTestStruct tsStart1(1, 2, 3);
        TTestStruct tsStart2(4, 5, 6);
        TTestStruct tsStart0(0, 0, 0);

        {
            TFile2DArrayVariableWriter<ui32> writer(TempFileName, sizeof(TTestStruct));
            writer.Write((const char*)&tsStart1);
            writer.NewLine();
            writer.Write((const char*)&tsStart2);
            writer.NewLine();
            writer.NewLine();
            writer.Finish();
        }
        TTestStruct ts1(Max<ui16>() - 1, Max<ui32>() - 1, Max<ui64>() - 1);
        TTestStruct ts2(Max<ui16>(), Max<ui32>(), Max<ui64>());

        {
            FileMapped2DArrayWritable<ui32, TTestStruct> reader(TempFileName);
            UNIT_ASSERT_EQUAL(reader.Size(), 4);
            UNIT_ASSERT_EQUAL(*reader[0], tsStart1);
            UNIT_ASSERT_EQUAL(*reader[1], tsStart2);
            UNIT_ASSERT_EQUAL(*reader[2], tsStart0);
            UNIT_ASSERT_EQUAL(*reader[3], tsStart0);

            UNIT_ASSERT(reader.SetAt(0, ts1));
            UNIT_ASSERT(reader.SetAt(1, ts2));
            UNIT_ASSERT_EQUAL(reader.SetAt(2, ts2), false);
            UNIT_ASSERT_EQUAL(reader.SetAt(3, ts2), false);
            UNIT_ASSERT_EQUAL(*reader[0], ts1);
            UNIT_ASSERT_EQUAL(*reader[1], ts2);
            reader.Term();
        }
        {
            FileMapped2DArrayWritable<ui32, TTestStruct> reader(TempFileName);
            UNIT_ASSERT_EQUAL(*reader[0], ts1);
            UNIT_ASSERT_EQUAL(*reader[1], ts2);
            UNIT_ASSERT_EQUAL(*reader[2], tsStart0);
            UNIT_ASSERT_EQUAL(*reader[3], tsStart0);
            reader.Term();
        }
    }

    Y_UNIT_TEST(TestUpdateStructure) {
        TTestStruct tsStart1(1, 2, 3);
        TTestStruct tsStart2(4, 5, 6);

        {
            TFile2DArrayVariableWriter<ui32> writer(TempFileName, sizeof(TTestStruct));
            writer.Write((const char*)&tsStart1);
            writer.NewLine();
            writer.Write((const char*)&tsStart2);
            writer.Finish();
        }

        TTestStruct ts1(Max<ui16>() - 1, Max<ui32>() - 1, Max<ui64>() - 1);
        TTestStruct ts2(Max<ui16>(), Max<ui32>(), Max<ui64>());
        TTestStructSmall ts1Small(Max<ui16>() - 1, Max<ui32>() - 1);
        TTestStructSmall ts2Small(Max<ui16>(), Max<ui32>());

        {
            FileMapped2DArrayWritable<ui32, TTestStruct> reader(TempFileName);
            UNIT_ASSERT_EQUAL(*reader[0], tsStart1);
            UNIT_ASSERT_EQUAL(*reader[1], tsStart2);

            UNIT_ASSERT(reader.SetAt(0, ts1));
            UNIT_ASSERT(reader.SetAt(1, ts2));
            UNIT_ASSERT_EQUAL(*reader[0], ts1);
            UNIT_ASSERT_EQUAL(*reader[1], ts2);
            reader.Term();
        }
        {
            FileMapped2DArrayWritable<ui32, TTestStruct> reader(TempFileName);
            UNIT_ASSERT_EQUAL(*reader[0], ts1);
            UNIT_ASSERT_EQUAL(*reader[1], ts2);
            reader.Term();
        }
        {
            FileMapped2DArrayWritable<ui32, TTestStructBig> reader(TempFileName, true);
            TTestStructBig ts0Big(1, 1, 1);
            TTestStructBig ts1Big(Max<ui16>() - 1, Max<ui32>() - 1, Max<ui64>() - 1);
            TTestStructBig ts2Big(Max<ui16>(), Max<ui32>(), Max<ui64>());
            UNIT_ASSERT_EQUAL(*reader[0], ts1Big);
            UNIT_ASSERT_EQUAL(*reader[1], ts2Big);
            UNIT_ASSERT_EQUAL(reader.SetAt(0, ts0Big), true);
            UNIT_ASSERT_EQUAL(*reader[0], ts0Big);
            UNIT_ASSERT_EQUAL(*reader[1], ts2Big);
            reader.Term();
        }
        {
            FileMapped2DArrayWritable<ui32, TTestStructBig> reader(TempFileName, true);
            TTestStructBig ts1Big(Max<ui16>() - 1, Max<ui32>() - 1, Max<ui64>() - 1);
            TTestStructBig ts2Big(Max<ui16>(), Max<ui32>(), Max<ui64>());
            UNIT_ASSERT_EQUAL(*reader[0], ts1Big);
            UNIT_ASSERT_EQUAL(*reader[1], ts2Big);
            reader.Term();
        }
        {
            FileMapped2DArrayWritable<ui32, TTestStructSmall> reader(TempFileName, true);
            TTestStructSmall ts0Small(1, 1);
            UNIT_ASSERT_EQUAL(*reader[0], ts1Small);
            UNIT_ASSERT_EQUAL(*reader[1], ts2Small);
            UNIT_ASSERT_EQUAL(reader.SetAt(0, ts0Small), true);
            UNIT_ASSERT_EQUAL(*reader[0], ts0Small);
            UNIT_ASSERT_EQUAL(*reader[1], ts2Small);
            reader.Term();
        }
        {
            FileMapped2DArrayWritable<ui32, TTestStructSmall> reader(TempFileName, true);
            TTestStructSmall ts0Small(1, 1);
            UNIT_ASSERT_EQUAL(*reader[0], ts1Small);
            UNIT_ASSERT_EQUAL(*reader[1], ts2Small);
            reader.Term();
        }
    }

    Y_UNIT_TEST(TestWriteUniversal) {
        TFile2DArrayWriter<ui64, char> writer(TempFileName, 0, true);
        Write(writer);
    }

    Y_UNIT_TEST(TestReadUniversal) {
        FileMapped2DArray<ui64, char> reader;
        Read(reader);
        reader.Term();
    }

    Y_UNIT_TEST(TestReadUniversalFromMapping) {
        FileMapped2DArray<ui64, char> reader;
        Read(reader, USE_MAP);
        reader.Term();
    }

    Y_UNIT_TEST(TestCleanup) {
        TTempFile tmpFile(TempFileName);
    }
};

Y_UNIT_TEST_SUITE(TFileMapped2DArraySizeMismatchTest) {
    static const char TempFileName[] = "./FileMapped2DArray-test";

    void CreateFile() {
        TFile2DArrayWriter<ui32, ui16> writer(TempFileName);
        // row 0
        writer.Write(0x0101);
        writer.Write(0x0202);
        // row 1
        writer.NewLine();
        writer.Write(0x0303);
        writer.Finish();
    }

    template <typename I, typename T>
    void ReadFailInit(FileMapped2DArray<I, T> & reader) {
        UNIT_ASSERT(!reader.Init(TempFileName));
    }

    template <typename T>
    T Conv(ui16 val) {
        T ret = 0;
        memcpy(&ret, &val, Min(sizeof val, sizeof(T)));
        return ret;
    }

    template <typename I, typename T>
    void Read(FileMapped2DArray<I, T> & reader, bool polite) {
        UNIT_ASSERT(reader.Init(TempFileName, polite));

        UNIT_ASSERT_VALUES_EQUAL(reader.Size(), 2u);
        // first row
        UNIT_ASSERT_EQUAL(reader[0], reader.GetBegin(0));
        UNIT_ASSERT_VALUES_EQUAL(reader.GetLength(0), 2u);
        UNIT_ASSERT_EQUAL(reader.GetBegin(0) + reader.GetLength(0), reader.GetEnd(0));
        UNIT_ASSERT_EQUAL(reader[0][0], Conv<T>(0x0101));
        UNIT_ASSERT_EQUAL(reader.GetAt(0, 1), Conv<T>(0x0202));
        // second row
        UNIT_ASSERT_EQUAL(reader[1], reader.GetBegin(1));
        UNIT_ASSERT_VALUES_EQUAL(reader.GetLength(1), 1u);
        UNIT_ASSERT_EQUAL(reader.GetBegin(1) + reader.GetLength(1), reader.GetEnd(1));
        UNIT_ASSERT_EQUAL(reader.GetAt(1, 0), Conv<T>(0x0303));
    }

    Y_UNIT_TEST(TestFailInit) {
        CreateFile();
        TTempFile tmpFile(TempFileName);
        FileMapped2DArray<ui32, ui32> reader;
        ReadFailInit(reader);
        reader.Term();
    }

    Y_UNIT_TEST(TestReadSameSize) {
        CreateFile();
        TTempFile tmpFile(TempFileName);
        FileMapped2DArray<ui32, ui16> reader;
        Read(reader, false);
        reader.Term();
    }

    Y_UNIT_TEST(TestReadUI8) {
        CreateFile();
        TTempFile tmpFile(TempFileName);
        FileMapped2DArray<ui32, ui8> reader;
        Read(reader, true);
        reader.Term();
    }

    Y_UNIT_TEST(TestReadUI32) {
        CreateFile();
        TTempFile tmpFile(TempFileName);
        FileMapped2DArray<ui32, ui32> reader;
        Read(reader, true);
        reader.Term();
    }
};
