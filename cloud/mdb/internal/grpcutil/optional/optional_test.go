package optional

import (
	"testing"

	"github.com/golang/protobuf/ptypes/wrappers"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/optional"
)

func TestStringFromGRPC(t *testing.T) {
	nothing := StringFromGRPC(nil)
	require.Equal(t, nothing, optional.String{})

	empty := StringFromGRPC(&wrappers.StringValue{Value: ""})
	require.Equal(t, empty, optional.NewString(""))

	text := StringFromGRPC(&wrappers.StringValue{Value: "asd"})
	require.Equal(t, text, optional.NewString("asd"))
}

func TestStringToGRPC(t *testing.T) {
	nothing := StringToGRPC(optional.String{})
	require.Nil(t, nothing)

	empty := StringToGRPC(optional.NewString(""))
	require.NotNil(t, empty)
	require.Equal(t, "", empty.GetValue())

	text := StringToGRPC(optional.NewString("asd"))
	require.NotNil(t, text)
	require.Equal(t, "asd", text.GetValue())
}
