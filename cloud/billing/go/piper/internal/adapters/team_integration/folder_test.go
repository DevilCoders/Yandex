package teamintegration

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	team_integration_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/team_integration"
)

type folderSuite struct {
	clientMockedSuite
	sess *Session
}

func TestFolder(t *testing.T) {
	suite.Run(t, new(folderSuite))
}

func (suite *folderSuite) SetupTest() {
	suite.clientMockedSuite.SetupTest()
	adapter, _ := New(suite.ctx, suite.mock)
	suite.sess = adapter.Session()
}

func (suite *folderSuite) TestResolve() {
	suite.mock.
		On("ResolveABC", mock.Anything, int64(1)).Return(team_integration_ic.ResolvedFolder{ID: 1, CloudID: "cld1", FolderID: "fld1"}, nil).
		On("ResolveABC", mock.Anything, int64(2)).Return(team_integration_ic.ResolvedFolder{ID: 2, CloudID: "cld2", FolderID: "fld2"}, nil).
		On("ResolveABC", mock.Anything, int64(3)).Return(team_integration_ic.ResolvedFolder{ID: 3, CloudID: "cld3", FolderID: "fld3"}, nil).
		On("ResolveABC", mock.Anything, int64(4)).Return(team_integration_ic.ResolvedFolder{ID: 4, CloudID: "cld4", FolderID: "fld4"}, nil)

	got, err := suite.sess.ResolveAbc(suite.ctx, entities.ProcessingScope{}, []int64{1, 2, 3, 4})

	suite.Require().NoError(err)
	want := []entities.AbcFolder{
		{AbcID: 1, AbcFolderID: "fld1", CloudID: "cld1"},
		{AbcID: 2, AbcFolderID: "fld2", CloudID: "cld2"},
		{AbcID: 3, AbcFolderID: "fld3", CloudID: "cld3"},
		{AbcID: 4, AbcFolderID: "fld4", CloudID: "cld4"},
	}
	suite.ElementsMatch(want, got)
	suite.mock.AssertExpectations(suite.T())
}

func (suite *folderSuite) TestResolveEmpty() {
	got, err := suite.sess.ResolveAbc(suite.ctx, entities.ProcessingScope{}, nil)

	suite.Require().NoError(err)
	suite.Empty(got)
}

func (suite *folderSuite) TestResolveCached() {
	suite.mock.On("ResolveABC", mock.Anything, int64(1)).
		Return(team_integration_ic.ResolvedFolder{ID: 1, CloudID: "cld1", FolderID: "fld1"}, nil).
		Once()
	suite.mock.On("ResolveABC", mock.Anything, int64(2)).
		Return(team_integration_ic.ResolvedFolder{ID: 2, CloudID: "cld2", FolderID: "fld2"}, nil).
		Once()

	_, err := suite.sess.ResolveAbc(suite.ctx, entities.ProcessingScope{}, []int64{1})
	suite.Require().NoError(err)

	got, err := suite.sess.ResolveAbc(suite.ctx, entities.ProcessingScope{}, []int64{1, 2})
	suite.Require().NoError(err)
	want := []entities.AbcFolder{
		{AbcID: 1, AbcFolderID: "fld1", CloudID: "cld1"},
		{AbcID: 2, AbcFolderID: "fld2", CloudID: "cld2"},
	}
	suite.ElementsMatch(want, got)
	suite.mock.AssertExpectations(suite.T())
}

func (suite *folderSuite) TestResolveErr() {
	suite.mock.On("ResolveABC", mock.Anything, int64(1)).Return(
		team_integration_ic.ResolvedFolder{}, errTest,
	)

	_, err := suite.sess.ResolveAbc(suite.ctx, entities.ProcessingScope{}, []int64{1})
	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
}
