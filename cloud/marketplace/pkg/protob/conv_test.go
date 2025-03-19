package protob

import (
	"encoding/json"
	"testing"

	"github.com/stretchr/testify/suite"
	"google.golang.org/protobuf/proto"
	"google.golang.org/protobuf/types/known/anypb"

	private "a.yandex-team.ru/cloud/bitbucket/private-api/yandex/cloud/priv/cdn/v1"
)

const (
	testFolderID = "test-folder-id-1"
)

func TestConversions(t *testing.T) {
	suite.Run(t, new(ConversionTestSuite))
}

type ConversionTestSuite struct {
	suite.Suite

	any          *anypb.Any
	protoMessage *private.ActivateProviderMetadata
}

func (suite *ConversionTestSuite) SetupSuite() {
	suite.protoMessage = &private.ActivateProviderMetadata{
		FolderId: testFolderID,
	}

	suite.any = MustBeAny(suite.protoMessage)
}

func (suite *ConversionTestSuite) TestConversionToJSON() {
	raw, err := JSONFromAnyMessage(suite.any)
	suite.Require().NoError(err)
	suite.Require().NotNil(raw)

	var any anypb.Any
	err = json.Unmarshal(raw, &any)
	suite.Require().NoError(err)

	protoMessage, err := any.UnmarshalNew()
	suite.Require().NoError(err)
	suite.Require().NotNil(protoMessage)

	suite.assertValidProtoMessage(protoMessage)
}

func (suite *ConversionTestSuite) TestConversionToMessage() {
	raw, err := json.Marshal(suite.any)
	suite.Require().NoError(err)
	suite.Require().NotEmpty(raw)

	any, err := AnyMessageFromJSON(raw)
	suite.Require().NoError(err)
	suite.Require().NotNil(any)

	protoMessage, err := any.UnmarshalNew()
	suite.Require().NoError(err)
	suite.Require().NotNil(protoMessage)

	suite.assertValidProtoMessage(protoMessage)
}

func (suite *ConversionTestSuite) TestMustBeAnySuccess() {
	protoMessage := MustBeAny(&private.ActivateProviderMetadata{
		FolderId: testFolderID,
	})

	suite.Require().NotNil(protoMessage)

	var typedMessage private.ActivateProviderMetadata
	err := protoMessage.UnmarshalTo(&typedMessage)
	suite.Require().NoError(err)
	suite.Require().Equal(testFolderID, typedMessage.FolderId)
}

func (suite *ConversionTestSuite) TestMustBeAnyPanic() {
	suite.Require().Panics(func() {
		_ = MustBeAny(nil)
	})
}

func (suite *ConversionTestSuite) assertValidProtoMessage(protoMessage proto.Message) {
	suite.Require().IsType(&private.ActivateProviderMetadata{}, protoMessage)

	suite.Require().Condition(func() bool {
		switch m := protoMessage.(type) {
		case *private.ActivateProviderMetadata:
			return m.FolderId == testFolderID
		default:
			suite.Require().Fail("unexpected proto message type %+v", m)
		}

		return false
	})

}
