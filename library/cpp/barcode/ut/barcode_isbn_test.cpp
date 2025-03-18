#include <library/cpp/barcode/barcode.h>

#include <library/cpp/testing/unittest/gtest.h>

TEST(BarcodeTest, Good_MaxLength) {
    TString original("12345678901234567890");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_EQ(original, result);
}

TEST(BarcodeTest, Good_WithSpace) {
    TString original("1 23 45 678 9012 3456 7890");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_EQ("12345678901234567890", result);
}

TEST(BarcodeTest, Good_X) {
    TString original("1234567890123456789X");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_EQ(original, result);
}

TEST(BarcodeTest, Good_x) {
    TString original("1234567890123456789x");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_EQ(original, result);
}

TEST(BarcodeTest, Bad_MaxLength) {
    TString original("123456789012345678901");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_TRUE(result.empty());
}

TEST(BarcodeTest, Bad_Chars_1) {
    TString original("1234568-0123456-8901");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_TRUE(result.empty());
}

TEST(BarcodeTest, Bad_Chars_2) {
    TString original("1234567890_345678901");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_TRUE(result.empty());
}

TEST(BarcodeTest, Bad_X) {
    TString original("123456789012X45678901");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_TRUE(result.empty());
}

TEST(BarcodeTest, Bad_x) {
    TString original("1234567890x345678901");
    TString result;
    result = TBarcode::ValidateBarcodeFast(original);
    EXPECT_TRUE(result.empty());
}

TEST(IsbnTest, Good_SBN) {
    TString original("9912158105");
    EXPECT_EQ(original, TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_SBN_Grouped_1) {
    TString original("991-2118-19-7");
    EXPECT_EQ(original, TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_SBN_Grouped_2) {
    TString original("99-12153-19-7");
    EXPECT_EQ(original, TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_SBN_Grouped_3) {
    TString original("99-1-215319-7");
    EXPECT_EQ(original, TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_SBN_X) {
    TString original("1-9921-5810-X");
    EXPECT_EQ("1-9921-5810-X", TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_SBN_x) {
    TString original("199-2-15810-x");
    EXPECT_EQ("199-2-15810-x", TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_SBN_Space) {
    TString original("1 -99 21-58 10-X ");
    EXPECT_EQ("1-9921-5810-X", TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Bad_SBN_Grouped) {
    TString original("1-99-21-5810-X ");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}

TEST(IsbnTest, Bad_SBN_CheckSum) {
    TString original("1-9921-5810-9");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}

TEST(IsbnTest, Bad_SBN_Symbol) {
    TString original("1-9921-581O-9");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}

TEST(IsbnTest, Bad_SBN_X) {
    TString original("1-992X-5810-0");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}

TEST(IsbnTest, Good_ISBN) {
    TString original("9781234567897");
    EXPECT_EQ(original, TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_ISBN979) {
    TString original("979-023-456789-9");
    EXPECT_EQ(original, TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Good_ISBN_Grouped) {
    TString original("978-1-234567-89-7");
    EXPECT_EQ(original, TBarcode::GetValidISBN(original));
}

TEST(IsbnTest, Bad_ISBN_Grouped) {
    TString original("978-12-345-67-89-7");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}

TEST(IsbnTest, Bad_ISBN_CheckSum) {
    TString original("978-12-34567-89-6");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}

TEST(IsbnTest, Bad_ISBN_Symbol) {
    TString original("978-O2-34567-89-0");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}

TEST(IsbnTest, Bad_ISBN_EAN) {
    TString original("976-02-34567-89-2");
    EXPECT_TRUE(TBarcode::GetValidISBN(original).empty());
}
