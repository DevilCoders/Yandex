#include <kernel/snippets/schemaorg/product_offer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/charset/wide.h>

using namespace NSchemaOrg;

Y_UNIT_TEST_SUITE(TOfferTests) {
    Y_UNIT_TEST(TestOfferIsNotAvailable) {
        TOffer offer;

        // false
        offer.Availability.clear();
        UNIT_ASSERT_EQUAL(false, offer.OfferIsNotAvailable());

        offer.Availability = u"http://schema.org/InStock";
        UNIT_ASSERT_EQUAL(false, offer.OfferIsNotAvailable());

        offer.Availability = u"http://schema.org/PreOrder";
        UNIT_ASSERT_EQUAL(false, offer.OfferIsNotAvailable());

        offer.Availability = u"OfStock";
        UNIT_ASSERT_EQUAL(false, offer.OfferIsNotAvailable());

        offer.Availability = u"нет";
        UNIT_ASSERT_EQUAL(false, offer.OfferIsNotAvailable());

        // true
        offer.Availability = u"http://schema.org/OutOfStock";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"http://schema.org/outofstock";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"http://schema.org/out_of_stock";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"OutOfStock";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"OutofStock";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"Out_Of_Stock";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"Out Of Stock";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"Товара нет в наличии";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"нет в наличии";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());

        offer.Availability = u"нет на складе";
        UNIT_ASSERT_EQUAL(true, offer.OfferIsNotAvailable());
    }

    static TString ParseOnlyCurrency(const TUtf16String& priceCurrency) {
        TPriceParsingResult result = ParsePrice(u"1", priceCurrency, "foo.ru");
        return result.IsValid ? result.CurrencyCode : TString();
    }

    static ui64 ParseOnlyPrice(const TUtf16String& price) {
        TPriceParsingResult result = ParsePrice(price, u"USD", "foo.com");
        return result.IsValid ? result.ParsedPriceMul100 : 0;
    }

    Y_UNIT_TEST(TestParsePrice) {
        TPriceParsingResult result;

        result = ParsePrice(u"1234.56", u"RUB", "foo.com");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(!result.IsLowPrice);
        UNIT_ASSERT_EQUAL(123456, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"1234.56", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("RUB", result.CurrencyCode);

        result = ParsePrice(u" 1 234 567.89 руб. ", u"RUR", "foo.com");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(!result.IsLowPrice);
        UNIT_ASSERT_EQUAL(123456789, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"1 234 567.89", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("RUB", result.CurrencyCode);

        result = ParsePrice(u"Цена от: 1 234 567 руб. ", u"", "foo.ru");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(result.IsLowPrice);
        UNIT_ASSERT_EQUAL(123456700, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"1 234 567", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("RUB", result.CurrencyCode);

        result = ParsePrice(u"от 1,234,567.89 рублей", u"руб.", "foo.ru");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(result.IsLowPrice);
        UNIT_ASSERT_EQUAL(123456789, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"1,234,567.89", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("RUB", result.CurrencyCode);

        result = ParsePrice(u"РУБ.99", u"руб.", "ru.foo.com");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(!result.IsLowPrice);
        UNIT_ASSERT_EQUAL(9900, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"99", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("RUB", result.CurrencyCode);

        result = ParsePrice(u"0,01", u"руб.", "foo.by");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(!result.IsLowPrice);
        UNIT_ASSERT_EQUAL(1, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"0,01", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("BYR", result.CurrencyCode);

        result = ParsePrice(u"$12.345.678,90", u"USD", "foo.com");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(!result.IsLowPrice);
        UNIT_ASSERT_EQUAL(1234567890, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"12.345.678,90", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("USD", result.CurrencyCode);

        result = ParsePrice(u"US $12345678.9", u"", "foo.com");
        UNIT_ASSERT(result.IsValid);
        UNIT_ASSERT(!result.IsLowPrice);
        UNIT_ASSERT_EQUAL(1234567890, result.ParsedPriceMul100);
        UNIT_ASSERT_EQUAL(u"12345678.9", result.FormattedPrice);
        UNIT_ASSERT_EQUAL("USD", result.CurrencyCode);

        UNIT_ASSERT(!ParsePrice(u"0", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"0.00", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"zzz 123", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"123 zzz", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"zzz 123 руб", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"123,00 124,00", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"123,00...124,00", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"123..124", u"RUB", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"12345.00", u"ZZZ", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"12345.00", u"KES", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"1.2345", u"USD", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"1.0000", u"USD", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"3. 059", u"USD", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"12345.00 РУБ", u"USD", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"01", u"USD", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"12 34", u"USD", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"12,345.678", u"USD", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"1", u"руб", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"1 руб", u"", "foo.com").IsValid);
        UNIT_ASSERT(!ParsePrice(u"RUB 1 RUB", u"", "foo.ru").IsValid);

        UNIT_ASSERT_EQUAL("RUB", ParseOnlyCurrency(u"RUB"));
        UNIT_ASSERT_EQUAL("RUB", ParseOnlyCurrency(u"RUR"));
        UNIT_ASSERT_EQUAL("RUB", ParseOnlyCurrency(u"Руб."));
        UNIT_ASSERT_EQUAL("RUB", ParseOnlyCurrency(u"р."));
        UNIT_ASSERT_EQUAL("RUB", ParseOnlyCurrency(u"рубля"));
        UNIT_ASSERT_EQUAL("UAH", ParseOnlyCurrency(u"UAH"));
        UNIT_ASSERT_EQUAL("USD", ParseOnlyCurrency(u"USD"));
        UNIT_ASSERT_EQUAL("USD", ParseOnlyCurrency(u"$"));
        UNIT_ASSERT_EQUAL("TRY", ParseOnlyCurrency(u"TRY"));
        UNIT_ASSERT_EQUAL("TRY", ParseOnlyCurrency(u"TL"));
        UNIT_ASSERT_EQUAL("EUR", ParseOnlyCurrency(u"EUR"));
        UNIT_ASSERT_EQUAL("BYR", ParseOnlyCurrency(u"BYR"));
        UNIT_ASSERT_EQUAL("KZT", ParseOnlyCurrency(u"KZT"));

        UNIT_ASSERT_EQUAL(100, ParseOnlyPrice(u"1"));
        UNIT_ASSERT_EQUAL(100, ParseOnlyPrice(u"1.00"));
        UNIT_ASSERT_EQUAL(1, ParseOnlyPrice(u"0.01"));
        UNIT_ASSERT_EQUAL(1, ParseOnlyPrice(u"0,01"));
        UNIT_ASSERT_EQUAL(1000001, ParseOnlyPrice(u"10.000,01"));
        UNIT_ASSERT_EQUAL(12345678901234502, ParseOnlyPrice(u"123.456.789.012.345,02"));
        UNIT_ASSERT_EQUAL(12345678901234500, ParseOnlyPrice(u"123456789012345"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123'456.78"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123`456.78"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123 456.78"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123,456.78"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123'456,78"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123`456,78"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123 456,78"));
        UNIT_ASSERT_EQUAL(12345678, ParseOnlyPrice(u"123.456,78"));
        UNIT_ASSERT_EQUAL(22233300, ParseOnlyPrice(u"222 333"));
        UNIT_ASSERT_EQUAL(233300, ParseOnlyPrice(u"2 333"));
        UNIT_ASSERT_EQUAL(10000, ParseOnlyPrice(u"100."));
        UNIT_ASSERT_EQUAL(10000, ParseOnlyPrice(u"100,-"));
    }
}
