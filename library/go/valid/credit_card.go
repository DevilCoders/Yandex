package valid

import (
	"errors"
	"strconv"
)

var (
	ErrInvalidCardPrefix = errors.New("valid: invalid credit card prefix")
)

// CreditCard check if the string is a credit card number.
// For all special cases see: http://www.regular-expressions.info/creditcard.html
func CreditCard(s string) error {
	return Luhn(s)
}

// VisaCard verifies Visa credit card number.
// All Visa card numbers start with a 4.
// New cards have 16 digits. Old cards have 13.
func VisaCard(s string) error {
	if len(s) != 13 && len(s) != 16 {
		return ErrInvalidStringLength
	}

	if s[0] != '4' {
		return ErrInvalidCardPrefix
	}

	return Luhn(s)
}

// MasterCard verifies Mastercard credit card number.
// MasterCard numbers either start with the numbers 51 through 55
// or with the numbers 2221 through 2720. All have 16 digits.
// There are Diners Club cards that begin with 5 and have 16 digits.
// These are a joint venture between Diners Club and MasterCard,
// and should be processed like a MasterCard.
func MasterCard(s string) error {
	if len(s) != 16 {
		return ErrInvalidStringLength
	}

	ft, _ := strconv.Atoi(s[:2])
	ff, _ := strconv.Atoi(s[:4])
	if (ft < 51 || ft > 55) && (ff < 2221 || ff > 2720) {
		return ErrInvalidCardPrefix
	}

	return Luhn(s)
}

// AmericanExpressCard verifies AmericanExpress credit card number.
// American Express card numbers start with 34 or 37 and have 15 digits.
func AmericanExpressCard(s string) error {
	if len(s) != 15 {
		return ErrInvalidStringLength
	}

	if s[:2] != "34" && s[:2] != "37" {
		return ErrInvalidCardPrefix
	}

	return Luhn(s)
}

// DinersClubCard verifies DinersClub credit card number.
// Diners Club card numbers begin with 300 through 305, 36 or 38.
// All have 14 digits.
func DinersClubCard(s string) error {
	if len(s) != 14 {
		return ErrInvalidStringLength
	}

	if s[:2] != "36" && s[:2] != "38" &&
		s[:3] != "300" && s[:3] != "301" &&
		s[:3] != "302" && s[:3] != "303" &&
		s[:3] != "304" && s[:3] != "305" {
		return ErrInvalidCardPrefix
	}

	return Luhn(s)
}

// DiscoverCard verifies Discover credit card number.
// Discover card numbers begin with 6011 or 65. All have 16 digits.
func DiscoverCard(s string) error {
	if len(s) != 16 {
		return ErrInvalidStringLength
	}

	if s[:2] != "65" && s[:4] != "6011" {
		return ErrInvalidCardPrefix
	}

	return Luhn(s)
}

// JCBCard verifies JCB credit card number.
// JCB cards beginning with 2131 or 1800 have 15 digits.
// JCB cards beginning with 35 have 16 digits.
func JCBCard(s string) error {
	if len(s) != 15 && len(s) != 16 {
		return ErrInvalidStringLength
	}

	if (s[:2] == "35" && len(s) == 16) ||
		((s[:4] == "2131" || s[:4] == "1800") && len(s) == 15) {
		return Luhn(s)
	}

	return ErrInvalidCardPrefix
}

// UnionPayCard verifies UnionPay credit card number.
// UnionPayCard cards beginning with 62, 6223 or 6270 and have 16 digits.
func UnionPayCard(s string) error {
	if len(s) != 16 {
		return ErrInvalidStringLength
	}

	if s[:2] != "62" && s[:4] != "6223" && s[:4] != "6270" {
		return ErrInvalidCardPrefix
	}

	return Luhn(s)
}
