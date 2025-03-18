package math

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func Test_MaxMinFunc(t *testing.T) {
	var inputs = []struct {
		Name        string
		A           int
		B           int
		ExpectedMax int
		ExpectedMin int
	}{
		{
			Name:        "first is bigger",
			A:           2,
			B:           1,
			ExpectedMax: 2,
			ExpectedMin: 1,
		},
		{
			Name:        "second is bigger",
			A:           1,
			B:           2,
			ExpectedMax: 2,
			ExpectedMin: 1,
		},
		{
			Name:        "negative comparison",
			A:           -5,
			B:           -7,
			ExpectedMax: -5,
			ExpectedMin: -7,
		},
		{
			Name:        "equal params",
			A:           42,
			B:           42,
			ExpectedMax: 42,
			ExpectedMin: 42,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			assert.Equal(t, MaxInt(input.A, input.B), input.ExpectedMax)
			assert.Equal(t, MinInt(input.A, input.B), input.ExpectedMin)
		})
	}
}
