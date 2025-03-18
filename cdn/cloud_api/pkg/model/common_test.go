package model

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestPagination(t *testing.T) {
	testCases := map[string]struct {
		page           Pagination
		expectedOffset int
		expectedLimit  int
	}{
		"1/20": {
			page: Pagination{
				PageToken: 1,
				PageSize:  20,
			},
			expectedOffset: 0,
			expectedLimit:  20,
		},
		"0/20": {
			page: Pagination{
				PageToken: 0,
				PageSize:  20,
			},
			expectedOffset: 0,
			expectedLimit:  20,
		},
		"2/20": {
			page: Pagination{
				PageToken: 2,
				PageSize:  20,
			},
			expectedOffset: 20,
			expectedLimit:  20,
		},
		"0/0": {
			page: Pagination{
				PageToken: 0,
				PageSize:  0,
			},
			expectedOffset: 0,
			expectedLimit:  20,
		},
		"1/120": {
			page: Pagination{
				PageToken: 1,
				PageSize:  120,
			},
			expectedOffset: 0,
			expectedLimit:  100,
		},
	}

	for testName, testCase := range testCases {
		testName, testCase := testName, testCase
		t.Run(testName, func(t *testing.T) {
			t.Parallel()

			assert.Equal(t, testCase.expectedOffset, testCase.page.Offset())
			assert.Equal(t, testCase.expectedLimit, testCase.page.Limit())
		})
	}
}
