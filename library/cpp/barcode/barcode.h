#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

//Proper barcode/ISBN formats:
//GTIN-14, EAN-13, EAN-8, UPC-A, UPC-E, ISBN, SBN

class TBarcode {
public:
    TBarcode(const TStringBuf& str) {
        Parse(str);
    }

    // original barcode, ex: 1-9921-5810-X
    inline const TString& Original() const {
        return Barcode;
    }

    // stripped version, ex: 199215810X
    inline const TString& Stripped() const {
        return StrippedBarcode;
    }

    bool Validate() const;
    bool ValidateFast() const; // very poor validation

    // Barcode validation used by Market. It simply checks barcode length
    // and does not verify checksum because some Market vendors produce invalid barcodes.
    bool ValidateLength() const;

    bool CheckISBNSize() const {
        // ISBN length can't be 12(UPC-A) or 8(EAN-8)
        return !EqualToOneOf(Barcode.size(), (size_t)0, (size_t)14, (size_t)12, (size_t)8);
    }

    // May indicate that this is an ISBN
    bool HasHyphen() const {
        return HasHyphenFlag;
    }

    static TString ValidateBarcodeFast(const TStringBuf& str) {
        TBarcode barcode(str);
        if (barcode.ValidateFast()) {
            return barcode.Original();
        } else {
            return "";
        }
    }

    static TString ValidateBarcode(const TStringBuf& str) {
        TBarcode barcode(str);
        if (barcode.Validate()) {
            return barcode.Original();
        } else {
            return "";
        }
    }

    static TString GetValidISBN(const TStringBuf& isbn) {
        TString validIsbn;
        TBarcode barcode(isbn);
        if (!barcode.Validate() || !barcode.CheckISBNSize())
            return "";
        return barcode.Original();
    }

private:
    void Parse(const TStringBuf& original);

    bool CheckDigits() const;
    bool CheckDigitsSBN() const;
    bool CheckDigitsEAN() const;
    bool CheckDigitsUPCE() const;
    bool CheckDigitsUPCA(const TVector<int>& digits) const;
    bool ValidateAsISBN() const;

    TString Barcode;
    TString StrippedBarcode;

    TVector<int> Digits;
    TVector<int> PartsLength;

    bool HasHyphenFlag = false;
};
