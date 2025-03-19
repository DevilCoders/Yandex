package zapjournald

import (
	"errors"
	"testing"
	"time"

	"github.com/stretchr/testify/suite"
	"go.uber.org/zap"
	"go.uber.org/zap/zapcore"
)

type encoderSuite struct {
	suite.Suite
}

func TestEncoder(t *testing.T) {
	suite.Run(t, new(encoderSuite))
}

func (suite *encoderSuite) String() string {
	return "suite"
}

func (suite *encoderSuite) MarshalLogArray(zapcore.ArrayEncoder) error {
	return nil
}

func (suite *encoderSuite) MarshalLogObject(zapcore.ObjectEncoder) error {
	return nil
}

func (suite *encoderSuite) TestStack() {
	enc := stringEncoder{}
	zap.Stack("k").AddTo(&enc)
	suite.Require().False(enc.notSupported)
	suite.Require().NotEmpty(enc.value)

	enc = stringEncoder{}
	zap.StackSkip("k", 2).AddTo(&enc)
	suite.Require().False(enc.notSupported)
	suite.Require().NotEmpty(enc.value)
}

func (suite *encoderSuite) TestSupported() {
	cases := []struct {
		name  string
		field zapcore.Field
		want  string
	}{
		{"Binary", zap.Binary("k", []byte("v")), "dg=="},
		{"Bool", zap.Bool("k", true), "true"},
		{"Bool", zap.Bool("k", false), "false"},
		{"Boolp", zap.Boolp("k", nil), "null"},
		{"ByteString", zap.ByteString("k", []byte("v")), "v"},
		{"Complex128p", zap.Complex128p("k", nil), "null"},
		{"Complex64p", zap.Complex64p("k", nil), "null"},
		{"Duration", zap.Duration("k", time.Second), "1"},
		{"Durationp", zap.Durationp("k", nil), "null"},
		{"Error", zap.Error(errors.New("err")), "err"},
		{"Float32", zap.Float32("k", 1), "1"},
		{"Float32p", zap.Float32p("k", nil), "null"},
		{"Float64", zap.Float64("k", 1), "1"},
		{"Float64p", zap.Float64p("k", nil), "null"},
		{"Int", zap.Int("k", 1), "1"},
		{"Int16", zap.Int16("k", 1), "1"},
		{"Int16p", zap.Int16p("k", nil), "null"},
		{"Int32", zap.Int32("k", 1), "1"},
		{"Int32p", zap.Int32p("k", nil), "null"},
		{"Int64", zap.Int64("k", 1), "1"},
		{"Int64p", zap.Int64p("k", nil), "null"},
		{"Int8", zap.Int8("k", 1), "1"},
		{"Int8p", zap.Int8p("k", nil), "null"},
		{"Intp", zap.Intp("k", nil), "null"},
		{"NamedError", zap.NamedError("k", errors.New("err")), "err"},
		{"Skip", zap.Skip(), ""},
		{"String", zap.String("k", "v"), "v"},
		{"Stringer", zap.Stringer("k", suite), "suite"},
		{"Stringp", zap.Stringp("k", nil), "null"},
		{"Time", zap.Time("k", time.Unix(0, 0).UTC()), "1970-01-01T00:00:00Z"},
		{"Timep", zap.Timep("k", nil), "null"},
		{"Uint", zap.Uint("k", 1), "1"},
		{"Uint16", zap.Uint16("k", 1), "1"},
		{"Uint16p", zap.Uint16p("k", nil), "null"},
		{"Uint32", zap.Uint32("k", 1), "1"},
		{"Uint32p", zap.Uint32p("k", nil), "null"},
		{"Uint64", zap.Uint64("k", 1), "1"},
		{"Uint64p", zap.Uint64p("k", nil), "null"},
		{"Uint8", zap.Uint8("k", 1), "1"},
		{"Uint8p", zap.Uint8p("k", nil), "null"},
		{"Uintp", zap.Uintp("k", nil), "null"},
		{"Uintptr", zap.Uintptr("k", 1), "1"},
		{"Uintptrp", zap.Uintptrp("k", nil), "null"},
	}
	for _, c := range cases {
		suite.Run(c.name, func() {
			enc := stringEncoder{}
			c.field.AddTo(&enc)
			suite.Require().False(enc.notSupported)
			suite.Require().Equal(c.want, enc.value)
		})
	}
}

func (suite *encoderSuite) TestUnSupported() {
	cases := []struct {
		name  string
		field zapcore.Field
	}{
		{"Any", zap.Any("k", struct{}{})},
		{"Array", zap.Array("k", suite)},
		{"Bools", zap.Bools("k", nil)},
		{"ByteStrings", zap.ByteStrings("k", nil)},
		{"Complex128", zap.Complex128("k", 0)},
		{"Complex128s", zap.Complex128s("k", nil)},
		{"Complex64", zap.Complex64("k", 0)},
		{"Complex64s", zap.Complex64s("k", nil)},
		{"Durations", zap.Durations("k", nil)},
		{"Errors", zap.Errors("k", nil)},
		{"Float32s", zap.Float32s("k", nil)},
		{"Float64s", zap.Float64s("k", nil)},
		{"Int16s", zap.Int16s("k", nil)},
		{"Int32s", zap.Int32s("k", nil)},
		{"Int64s", zap.Int64s("k", nil)},
		{"Int8s", zap.Int8s("k", nil)},
		{"Ints", zap.Ints("k", nil)},
		{"Namespace", zap.Namespace("k")},
		{"Object", zap.Object("k", suite)},
		{"Reflect", zap.Reflect("k", struct{}{})},
		{"Strings", zap.Strings("k", nil)},
		{"Times", zap.Times("k", nil)},
		{"Uint16s", zap.Uint16s("k", nil)},
		{"Uint32s", zap.Uint32s("k", nil)},
		{"Uint64s", zap.Uint64s("k", nil)},
		{"Uint8s", zap.Uint8s("k", nil)},
		{"Uintptrs", zap.Uintptrs("k", nil)},
		{"Uints", zap.Uints("k", nil)},
	}
	for _, c := range cases {
		suite.Run(c.name, func() {
			enc := stringEncoder{}
			c.field.AddTo(&enc)
			suite.Require().True(enc.notSupported)
		})
	}
}
