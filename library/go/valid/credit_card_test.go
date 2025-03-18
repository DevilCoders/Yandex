package valid_test

import (
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/valid"
)

func TestCreditCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrEmptyString},
		{"foo", valid.ErrInvalidChecksum},
		{"5398228707871528", valid.ErrInvalidChecksum},

		{"375556917985515", nil},
		{"36050234196908", nil},
		{"4716461583322103", nil},
		{"5398228707871527", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.CreditCard(tc.param))
		})
	}
}

func TestVisaCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"foo", valid.ErrInvalidStringLength},
		{"5398228707871528", valid.ErrInvalidCardPrefix},
		{"4716213139245218", valid.ErrInvalidChecksum},

		{"4716213139245217", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			t.Run(tc.param, func(t *testing.T) {
				assert.Equal(t, tc.expectErr, valid.VisaCard(tc.param))
			})
		})
	}
}

func TestMasterCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"foo", valid.ErrInvalidStringLength},
		{"375556917985515", valid.ErrInvalidStringLength},
		{"3755569179855152", valid.ErrInvalidCardPrefix},
		{"5515805738324651", valid.ErrInvalidChecksum},

		{"5515805738324655", nil},
		{"5309309013152196", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.MasterCard(tc.param))
		})
	}
}

func TestAmericanExpressCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"foo", valid.ErrInvalidStringLength},
		{"530930901315219", valid.ErrInvalidCardPrefix},
		{"349822870787152", valid.ErrInvalidChecksum},

		{"375556917985515", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.AmericanExpressCard(tc.param))
		})
	}
}

func TestDinersClubCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"foo", valid.ErrInvalidStringLength},
		{"53093090131521", valid.ErrInvalidCardPrefix},
		{"38555691798551", valid.ErrInvalidChecksum},

		{"30060129447551", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.DinersClubCard(tc.param))
		})
	}
}

func TestDiscoverCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"foo", valid.ErrInvalidStringLength},
		{"5309309013152196", valid.ErrInvalidCardPrefix},
		{"6011229282505482", valid.ErrInvalidChecksum},

		{"6011229282505485", nil},
		{"6011748439365527", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.DiscoverCard(tc.param))
		})
	}
}

func TestJCBCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"foo", valid.ErrInvalidStringLength},
		{"375556917985515", valid.ErrInvalidCardPrefix},
		{"180036877154341", valid.ErrInvalidChecksum},

		{"180036877154241", nil},
		{"3533868143240232", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.JCBCard(tc.param))
		})
	}
}

func TestUnionPayCard(t *testing.T) {
	var testCases = []struct {
		param     string
		expectErr error
	}{
		{"", valid.ErrInvalidStringLength},
		{"foo", valid.ErrInvalidStringLength},
		{"6011748439365527", valid.ErrInvalidCardPrefix},
		{"6247708070585850", valid.ErrInvalidChecksum},

		{"6247708070585854", nil},
		{"6271182418173113", nil},
		{"6223187683418217", nil},
		{"6270503846127135", nil},
	}
	for _, tc := range testCases {
		t.Run(tc.param, func(t *testing.T) {
			assert.Equal(t, tc.expectErr, valid.UnionPayCard(tc.param))
		})
	}
}
