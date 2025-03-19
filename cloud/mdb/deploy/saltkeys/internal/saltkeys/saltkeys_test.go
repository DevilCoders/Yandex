package saltkeys_test

import (
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys"
	"a.yandex-team.ru/cloud/mdb/deploy/saltkeys/internal/saltkeys/mocks"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestMinions(t *testing.T) {
	errMock := xerrors.NewSentinel("mock error")
	inputs := []struct {
		name        string
		minionsList map[saltkeys.StateID][]string
		errList     error
		minionsOut  map[string][]saltkeys.StateID
		errOut      error
	}{
		{
			name:        "empty minions",
			minionsList: map[saltkeys.StateID][]string{},
			minionsOut:  map[string][]saltkeys.StateID{},
		},
		{
			name: "error",
			minionsList: map[saltkeys.StateID][]string{
				saltkeys.Pre: {"does not matter"},
			},
			errList: errMock,
			errOut:  errMock,
		},
		{
			name: "one type",
			minionsList: map[saltkeys.StateID][]string{
				saltkeys.Pre: {"pre1", "pre2"},
			},
			minionsOut: map[string][]saltkeys.StateID{
				"pre1": {saltkeys.Pre},
				"pre2": {saltkeys.Pre},
			},
		},
		{
			name: "all types",
			minionsList: map[saltkeys.StateID][]string{
				saltkeys.Pre:      {"pre1", "pre2"},
				saltkeys.Accepted: {"accepted1", "accepted2"},
				saltkeys.Rejected: {"rejected1", "rejected2"},
				saltkeys.Denied:   {"denied1", "denied2"},
			},
			minionsOut: map[string][]saltkeys.StateID{
				"pre1":      {saltkeys.Pre},
				"pre2":      {saltkeys.Pre},
				"accepted1": {saltkeys.Accepted},
				"accepted2": {saltkeys.Accepted},
				"rejected1": {saltkeys.Rejected},
				"rejected2": {saltkeys.Rejected},
				"denied1":   {saltkeys.Denied},
				"denied2":   {saltkeys.Denied},
			},
		},
		{
			name: "multiple types",
			minionsList: map[saltkeys.StateID][]string{
				saltkeys.Pre:      {"pre", "multiple"},
				saltkeys.Accepted: {"accepted", "multiple"},
				saltkeys.Rejected: {"rejected", "multiple"},
				saltkeys.Denied:   {"denied", "multiple"},
			},
			minionsOut: map[string][]saltkeys.StateID{
				"pre":      {saltkeys.Pre},
				"accepted": {saltkeys.Accepted},
				"rejected": {saltkeys.Rejected},
				"denied":   {saltkeys.Denied},
				"multiple": {saltkeys.Pre, saltkeys.Accepted, saltkeys.Rejected, saltkeys.Denied},
			},
		},
	}

	for _, input := range inputs {
		t.Run(input.name, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			skeys := mocks.NewMockKeys(ctrl)

			var states []saltkeys.StateID
			// Specify returned minions and error
			for state, fqdns := range input.minionsList {
				skeys.EXPECT().List(state).Return(fqdns, input.errList)
				states = append(states, state)
			}

			minions, err := saltkeys.Minions(skeys, states...)
			if input.errOut != nil {
				assert.Error(t, err)
				assert.True(t, xerrors.Is(err, input.errOut))
				assert.Len(t, minions, 0)
			} else {
				assert.NoError(t, err)

				assert.Len(t, minions, len(input.minionsOut))
				for k, expected := range input.minionsOut {
					actual, ok := minions[k]
					assert.True(t, ok)
					assert.ElementsMatch(t, expected, actual)
				}
			}
		})
	}
}
