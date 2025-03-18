// based on: http://en.wikipedia.org/wiki/ISBN
//           http://en.wikipedia.org/wiki/Barcode

#include "barcode.h"

#include <util/string/strip.h>
#include <util/string/ascii.h>

#include <utility>

bool TBarcode::CheckDigitsSBN() const {
    Y_ASSERT(Digits.size() == 10); // 10-digit isbn (SBN)
    int check = 0;
    int i = 10;
    for (auto digit : Digits)
        check += (i--) * digit;
    return check % 11 == 0;
}

bool TBarcode::CheckDigitsUPCA(const TVector<int>& digits) const {
    //14-digit barcode (GTIN-14), 12-digit barcode (UPC-A) and 8-digit barcode (EAN-8)
    Y_ASSERT(Digits.size() == 14 || Digits.size() == 12 || Digits.size() == 8);
    int check = 0;
    for (auto it = digits.begin(); it < digits.end(); it += 2)
        check += 3 * (*it);

    for (auto it = digits.begin() + 1; it < digits.end(); it += 2)
        check += *it;

    return check % 10 == 0;
}

bool TBarcode::CheckDigitsUPCE() const {
    Y_ASSERT(Digits.size() == 8); //8-digit barcode (UPC-E)
    TVector<int> upca(Digits);
    TVector<int>::iterator it = upca.begin();

    switch (Digits.at(6)) {
        case 0:
            upca.insert(it + 3, 5, 0);
            break;
        case 1:
            it = upca.insert(it + 3, 1);
            upca.insert(it + 1, 4, 0);
            break;
        case 2:
            it = upca.insert(it + 3, 2);
            upca.insert(it + 1, 4, 0);
            break;
        case 3:
            upca.insert(it + 4, 5, 0);
            break;
        case 4:
            upca.insert(it + 5, 5, 0);
            break;
        case 5:
            upca.insert(it + 6, 4, 0);
            it = upca.begin();
            upca.insert(it + 10, 5);
            break;
        case 6:
            upca.insert(it + 6, 4, 0);
            it = upca.begin();
            upca.insert(it + 10, 6);
            break;
        case 7:
            upca.insert(it + 6, 4, 0);
            it = upca.begin();
            upca.insert(it + 10, 7);
            break;
        case 8:
            upca.insert(it + 6, 4, 0);
            it = upca.begin();
            upca.insert(it + 10, 8);
            break;
        case 9:
            upca.insert(it + 6, 4, 0);
            it = upca.begin();
            upca.insert(it + 10, 9);
            break;
    }
    upca.erase(upca.end() - 2);
    return CheckDigitsUPCA(upca);
}

bool TBarcode::CheckDigitsEAN() const {
    Y_ASSERT(Digits.size() == 13); // 13-digit isbn/barcode (EAN)
    int check = 0;
    for (auto it = Digits.begin(); it < Digits.end(); it += 2)
        check += *it;

    for (auto it = Digits.begin() + 1; it < Digits.end(); it += 2)
        check += 3 * (*it);

    return check % 10 == 0;
}

bool TBarcode::CheckDigits() const {
    switch (Digits.size()) {
        // 10-digit isbn (SBN)
        case 10:
            return CheckDigitsSBN();
        // 12-digit barcode (UPC-A) and 8-digit barcode (EAN-8)
        case 8:
            if (CheckDigitsUPCA(Digits))
                return true;
            else
                return CheckDigitsUPCE();
        case 12:
        case 14:
            return CheckDigitsUPCA(Digits);
        // 13-digit isbn/barcode (EAN)
        case 13:
            return CheckDigitsEAN();
    }
    return false;
}

void TBarcode::Parse(const TStringBuf& original) {
    Digits.reserve(14);     // max valid barcode digits count
    PartsLength.reserve(5); // approx max real barcode parts size
    PartsLength.push_back(0);
    TStringBuf str = StripString(original);
    for (TStringBuf::const_iterator currSymbol = str.begin(); currSymbol != str.end(); ++currSymbol) {
        if (IsAsciiDigit(*currSymbol)) {
            Barcode += *currSymbol;
            StrippedBarcode += *currSymbol;
            ++PartsLength.back();
            Digits.push_back(*currSymbol - '0');
            continue;
        }
        if (*currSymbol == '-') {
            Barcode += *currSymbol;
            PartsLength.push_back(0);
            HasHyphenFlag = true;
            continue;
        }
        if (*currSymbol == ' ')
            continue;
        if ((*currSymbol == 'x' || *currSymbol == 'X') && (currSymbol + 1 == str.end())) {
            ++PartsLength.back();
            Digits.push_back(10);
            Barcode += *currSymbol;
            StrippedBarcode += *currSymbol;
            continue;
        }
        Digits.clear();
        PartsLength.clear();
        Barcode.clear();
        StrippedBarcode.clear();
        break;
    }
}

bool TBarcode::ValidateAsISBN() const {
    const static size_t maxPartsNum = 5;
    // Requirements for length of EAN, group id, publisher and title parts
    const static std::pair<int, int> partsLenReq[maxPartsNum] = {
        std::make_pair(0, 3), // EAN. First min length, second max length
        std::make_pair(1, 5), // Publisher. First min length, second max length
        std::make_pair(1, 7), // Group ID. First min length, second max length
        std::make_pair(1, 7), // Title. First min length, second max length
        std::make_pair(1, 1)  // Check digit. First min length, second max length
    };

    // CHECK FOR ISBN(EAN) AND ISBN(SBN)
    if (PartsLength.size() > maxPartsNum)
        return false;

    // check that the isbn has correct length of
    // group id, publisher, title and check digit
    int i = maxPartsNum;
    for (TVector<int>::const_reverse_iterator it = PartsLength.rbegin(); it != PartsLength.rend(); ++it) {
        --i;
        if (*it > partsLenReq[i].second || *it < partsLenReq[i].first)
            return false;
    }

    // check that the isbn has correct EAN
    if (Digits.size() == 13 && (PartsLength.at(0) != partsLenReq[0].second ||
                                (!Barcode.StartsWith(TStringBuf("978")) &&
                                 !Barcode.StartsWith(TStringBuf("979")))))
        return false;
    return true;
}

bool TBarcode::Validate() const {
    // CHECK FOR ISBN/BARCODE

    // check the the isbn/barcode has correct check digit
    if (!CheckDigits())
        return false;

    // this check is for isbn/barcode without delimiter '-' for parts
    if (PartsLength.size() == 1)
        return true;

    if (Digits.size() == 12 || Digits.size() == 8)
        return false;

    return ValidateAsISBN();
}

bool TBarcode::ValidateFast() const {
    return !Barcode.empty() && Barcode.length() <= 20 && PartsLength.size() <= 1;
}

bool TBarcode::ValidateLength() const {
    return EqualToOneOf(Digits.size(), 8u, 10u, 12u, 13u, 14u);
}
