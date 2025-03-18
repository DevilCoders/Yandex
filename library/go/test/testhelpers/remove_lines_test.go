package testhelpers

import (
	"testing"

	"github.com/stretchr/testify/require"
)

func TestRemoveLines(t *testing.T) {
	inputs := []struct {
		Name   string
		String string
		Lines  []int
		Output string
		Error  bool
	}{
		{
			Name: "Empty",
		},
		{
			Name:   "RemoveSingle",
			String: "foo\n",
			Lines:  []int{0},
		},
		{
			Name:   "RemoveStart",
			String: "start\nmiddle\nend\n",
			Lines:  []int{0},
			Output: "middle\nend\n",
		},
		{
			Name:   "RemoveMiddle",
			String: "start\nmiddle\nend\n",
			Lines:  []int{1},
			Output: "start\nend\n",
		},
		{
			Name:   "RemoveEnd",
			String: "start\nmiddle\nend\n",
			Lines:  []int{2},
			Output: "start\nmiddle\n",
		},
		{
			Name:   "RemoveStartAndMiddle",
			String: "start\nmiddle\nend\n",
			Lines:  []int{0, 1},
			Output: "end\n",
		},
		{
			Name:   "RemoveMiddleAndEnd",
			String: "start\nmiddle\nend\n",
			Lines:  []int{1, 2},
			Output: "start\n",
		},
		{
			Name:   "RemoveStardAndEnd",
			String: "start\nmiddle\nend\n",
			Lines:  []int{0, 2},
			Output: "middle\n",
		},
		{
			Name:   "RemoveAll",
			String: "start\nmiddle\nend\n",
			Lines:  []int{0, 1, 2},
		},
		{
			Name:   "RemoveEmpty",
			String: "",
			Lines:  []int{0},
			Error:  true,
		},
		{
			Name:   "RemoveBeforeStart",
			String: "start\nmiddle\nend\n",
			Lines:  []int{-1},
			Error:  true,
		},
		{
			Name:   "RemoveAfterEnd",
			String: "start\nmiddle\nend\n",
			Lines:  []int{3},
			Error:  true,
		},
	}

	for _, input := range inputs {
		t.Run(input.Name, func(t *testing.T) {
			out, err := RemoveLines(input.String, input.Lines...)
			if input.Error {
				require.Error(t, err)
				require.Equal(t, input.String, out)
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.Output, out)
		})
	}
}
