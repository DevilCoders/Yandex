package iam

import (
	"testing"

	"github.com/stretchr/testify/mock"
	"github.com/stretchr/testify/suite"

	"a.yandex-team.ru/cloud/billing/go/piper/internal/core/entities"
	iam_ic "a.yandex-team.ru/cloud/billing/go/piper/internal/interconnect/iam"
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
	suite.mock.On("ResolveFolder", mock.Anything, "fld1", "fld2", "fld3", "fld4").Return(
		[]iam_ic.ResolvedFolder{
			{ID: "fld1", CloudID: "cld1"},
			{ID: "fld2", CloudID: "cld2"},
			{ID: "fld3", CloudID: "cld3"},
			{ID: "fld4", CloudID: "cld4"},
		}, nil,
	).Once()

	got, err := suite.sess.GetFolders(suite.ctx, entities.ProcessingScope{}, []string{
		"fld1", "fld2", "fld3", "fld4",
	})

	suite.Require().NoError(err)
	want := []entities.Folder{
		{FolderID: "fld1", CloudID: "cld1"},
		{FolderID: "fld2", CloudID: "cld2"},
		{FolderID: "fld3", CloudID: "cld3"},
		{FolderID: "fld4", CloudID: "cld4"},
	}
	suite.ElementsMatch(want, got)
	suite.mock.AssertExpectations(suite.T())
}

func (suite *folderSuite) TestResolveEmpty() {
	got, err := suite.sess.GetFolders(suite.ctx, entities.ProcessingScope{}, []string{})

	suite.Require().NoError(err)
	suite.Empty(got)
}

func (suite *folderSuite) TestResolveCached() {
	suite.mock.On("ResolveFolder", mock.Anything, "fld").Return(
		[]iam_ic.ResolvedFolder{{ID: "fld", CloudID: "cld"}}, nil,
	).Once()
	suite.mock.On("ResolveFolder", mock.Anything, "fld_other").Return(
		[]iam_ic.ResolvedFolder{{ID: "fld_other", CloudID: "cld"}}, nil,
	).Once()

	_, err := suite.sess.GetFolders(suite.ctx, entities.ProcessingScope{}, []string{"fld"})
	suite.Require().NoError(err)

	got, err := suite.sess.GetFolders(suite.ctx, entities.ProcessingScope{}, []string{"fld", "fld_other"})
	suite.Require().NoError(err)
	want := []entities.Folder{
		{FolderID: "fld", CloudID: "cld"},
		{FolderID: "fld_other", CloudID: "cld"},
	}
	suite.ElementsMatch(want, got)
	suite.mock.AssertExpectations(suite.T())
}

func (suite *folderSuite) TestResolveErr() {
	suite.mock.On("ResolveFolder", mock.Anything, "fld").Return(
		[]iam_ic.ResolvedFolder{}, errTest,
	)

	_, err := suite.sess.GetFolders(suite.ctx, entities.ProcessingScope{}, []string{"fld"})
	suite.Require().Error(err)
	suite.ErrorIs(err, errTest)
}

func (suite *folderSuite) TestResolveBatches() {
	suite.sess.batchSizeOverride = 1
	suite.mock.
		On("ResolveFolder", mock.Anything, "fld1").Return([]iam_ic.ResolvedFolder{{ID: "fld1", CloudID: "cld1"}}, nil).
		On("ResolveFolder", mock.Anything, "fld2").Return([]iam_ic.ResolvedFolder{{ID: "fld2", CloudID: "cld2"}}, nil).
		On("ResolveFolder", mock.Anything, "fld3").Return([]iam_ic.ResolvedFolder{{ID: "fld3", CloudID: "cld3"}}, nil).
		On("ResolveFolder", mock.Anything, "fld4").Return([]iam_ic.ResolvedFolder{{ID: "fld4", CloudID: "cld4"}}, nil)

	got, err := suite.sess.GetFolders(suite.ctx, entities.ProcessingScope{}, []string{
		"fld1", "fld2", "fld3", "fld4",
	})

	suite.Require().NoError(err)
	want := []entities.Folder{
		{FolderID: "fld1", CloudID: "cld1"},
		{FolderID: "fld2", CloudID: "cld2"},
		{FolderID: "fld3", CloudID: "cld3"},
		{FolderID: "fld4", CloudID: "cld4"},
	}
	suite.ElementsMatch(want, got)
	suite.mock.AssertExpectations(suite.T())
}
