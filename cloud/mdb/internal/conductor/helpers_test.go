package conductor_test

import (
	"context"
	"fmt"
	"testing"

	"github.com/golang/mock/gomock"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/mocks"
	"a.yandex-team.ru/cloud/mdb/internal/optional"
	"a.yandex-team.ru/library/go/core/xerrors"
)

func TestParseGroup(t *testing.T) {
	var inputs = []struct {
		In  string
		Out conductor.ParsedGroup
		Ok  bool
		Err error
	}{
		{
			In:  "%test",
			Out: conductor.ParsedGroup{Name: "test"},
			Ok:  true,
		},
		{
			In:  "%test@vla",
			Out: conductor.ParsedGroup{Name: "test", DC: optional.NewString("vla")},
			Ok:  true,
		},
		{
			In: "",
		},
		{
			In: "test",
		},
		{
			In:  "%",
			Err: conductor.ErrEmptyGroup,
		},
		{
			In:  "%@",
			Err: conductor.ErrEmptyGroup,
		},
		{
			In:  "%test@",
			Err: conductor.ErrEmptyDC,
		},
	}

	for _, input := range inputs {
		t.Run(input.In, func(t *testing.T) {
			res, ok, err := conductor.ParseGroup(input.In)

			if !input.Ok {
				assert.False(t, ok)
				if input.Err != nil {
					assert.Error(t, err)
					assert.True(t, xerrors.Is(err, input.Err))
				}

				return
			}

			assert.True(t, ok)
			assert.Equal(t, input.Out, res)

		})
	}
}

func TestParseTarget(t *testing.T) {
	ctx := context.Background()
	testErr := xerrors.New("test error")

	var inputs = []struct {
		In   string
		Out  conductor.ParsedTarget
		Mock func(client *mocks.MockClient)
		Err  error
	}{
		{
			In:  "fqdn",
			Out: conductor.ParsedTarget{Hosts: []string{"fqdn"}},
		},
		{
			In:  "-fqdn",
			Out: conductor.ParsedTarget{Hosts: []string{"fqdn"}, Negative: true},
		},
		{
			In:  "%group",
			Out: conductor.ParsedTarget{Hosts: []string{"groupfqdn1", "groupfqdn2"}},
			Mock: func(client *mocks.MockClient) {
				client.EXPECT().GroupToHosts(gomock.Any(), "group", conductor.GroupToHostsAttrs{}).Return([]string{"groupfqdn1", "groupfqdn2"}, nil)
			},
		},
		{
			In:  "-%group",
			Out: conductor.ParsedTarget{Hosts: []string{"groupfqdn1", "groupfqdn2"}, Negative: true},
			Mock: func(client *mocks.MockClient) {
				client.EXPECT().GroupToHosts(gomock.Any(), "group", conductor.GroupToHostsAttrs{}).Return([]string{"groupfqdn1", "groupfqdn2"}, nil)
			},
		},
		{
			In:  "%",
			Err: conductor.ErrEmptyGroup,
		},
		{
			In: "%group",
			Mock: func(client *mocks.MockClient) {
				client.EXPECT().GroupToHosts(gomock.Any(), "group", conductor.GroupToHostsAttrs{}).Return(nil, testErr)
			},
			Err: testErr,
		},
	}

	for _, input := range inputs {
		t.Run(input.In, func(t *testing.T) {
			ctrl := gomock.NewController(t)
			client := mocks.NewMockClient(ctrl)

			if input.Mock != nil {
				input.Mock(client)
			}

			target, err := conductor.ParseTarget(ctx, client, input.In)

			if input.Err != nil {
				assert.Error(t, err)
				assert.True(t, xerrors.Is(err, input.Err))
				return
			}

			require.NoError(t, err)
			require.Equal(t, input.Out, target)
		})
	}
}

func TestParseMultiTarget(t *testing.T) {
	ctx := context.Background()
	testErr := xerrors.New("test error")

	var inputs = []struct {
		In   string
		Out  []conductor.ParsedTarget
		Mock func(client *mocks.MockClient)
		Err  error
	}{
		{
			In: "fqdn,%group,-fqdn2",
			Out: []conductor.ParsedTarget{
				{Hosts: []string{"fqdn"}},
				{Hosts: []string{"groupfqdn1", "groupfqdn2"}},
				{Hosts: []string{"fqdn2"}, Negative: true},
			},
			Mock: func(client *mocks.MockClient) {
				client.EXPECT().GroupToHosts(gomock.Any(), "group", conductor.GroupToHostsAttrs{}).Return([]string{"groupfqdn1", "groupfqdn2"}, nil)
			},
		},
		{
			In: "fqdn,%group",
			Mock: func(client *mocks.MockClient) {
				client.EXPECT().GroupToHosts(gomock.Any(), "group", conductor.GroupToHostsAttrs{}).Return(nil, testErr)
			},
			Err: testErr,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprint(input.In), func(t *testing.T) {
			ctrl := gomock.NewController(t)
			client := mocks.NewMockClient(ctrl)

			if input.Mock != nil {
				input.Mock(client)
			}

			targets, err := conductor.ParseMultiTarget(ctx, client, input.In)

			if input.Err != nil {
				assert.Error(t, err)
				assert.True(t, xerrors.Is(err, input.Err))
				return
			}

			require.NoError(t, err)
			require.ElementsMatch(t, input.Out, targets)
		})
	}
}

func TestParseMultiTargets(t *testing.T) {
	ctx := context.Background()
	testErr := xerrors.New("test error")

	var inputs = []struct {
		In   []string
		Out  []conductor.ParsedTarget
		Mock func(client *mocks.MockClient)
		Err  error
	}{
		{
			In: []string{"fqdn,%group", "-fqdn2"},
			Out: []conductor.ParsedTarget{
				{Hosts: []string{"fqdn"}},
				{Hosts: []string{"groupfqdn1", "groupfqdn2"}},
				{Hosts: []string{"fqdn2"}, Negative: true},
			},
			Mock: func(client *mocks.MockClient) {
				client.EXPECT().GroupToHosts(gomock.Any(), "group", conductor.GroupToHostsAttrs{}).Return([]string{"groupfqdn1", "groupfqdn2"}, nil)
			},
		},
		{
			In: []string{"fqdn,%group"},
			Mock: func(client *mocks.MockClient) {
				client.EXPECT().GroupToHosts(gomock.Any(), "group", conductor.GroupToHostsAttrs{}).Return(nil, testErr)
			},
			Err: testErr,
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprint(input.In), func(t *testing.T) {
			ctrl := gomock.NewController(t)
			client := mocks.NewMockClient(ctrl)

			if input.Mock != nil {
				input.Mock(client)
			}

			targets, err := conductor.ParseMultiTargets(ctx, client, input.In...)

			if input.Err != nil {
				assert.Error(t, err)
				assert.True(t, xerrors.Is(err, input.Err))
				return
			}

			require.NoError(t, err)
			require.ElementsMatch(t, input.Out, targets)
		})
	}
}

func TestCollapseTargets(t *testing.T) {
	var inputs = []struct {
		In  []conductor.ParsedTarget
		Out []string
	}{
		{
			In: []conductor.ParsedTarget{
				{Hosts: []string{"fqdn"}},
				{Hosts: []string{"groupfqdn3"}, Negative: true},
				{Hosts: []string{"groupfqdn1", "groupfqdn2"}},
				{Hosts: []string{"groupfqdn2", "groupfqdn3"}},
				{Hosts: []string{"fqdn", "groupfqdn2"}, Negative: true},
			},
			Out: []string{"groupfqdn1"},
		},
	}

	for _, input := range inputs {
		t.Run(fmt.Sprint(input.In), func(t *testing.T) {
			require.ElementsMatch(t, input.Out, conductor.CollapseTargets(input.In))
		})
	}
}
