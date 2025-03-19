package zapjournald

import (
	"encoding/base64"
	"errors"
	"math"
	"strconv"
	"time"

	"go.uber.org/zap/zapcore"
)

var errNotSupported = errors.New("field is not supported")

type stringEncoder struct {
	value        string
	notSupported bool
}

var _ zapcore.ObjectEncoder = &stringEncoder{}

func (enc *stringEncoder) AddArray(_ string, arr zapcore.ArrayMarshaler) error {
	enc.notSupported = true
	return nil
}

func (enc *stringEncoder) AddObject(_ string, obj zapcore.ObjectMarshaler) error {
	enc.notSupported = true
	return nil
}

func (enc *stringEncoder) AddBinary(key string, val []byte) {
	enc.AddString(key, base64.StdEncoding.EncodeToString(val))
}

func (enc *stringEncoder) AddByteString(_ string, val []byte) {
	enc.value = string(val)
}

func (enc *stringEncoder) AddBool(_ string, val bool) {
	if val {
		enc.value = "true"
		return
	}
	enc.value = "false"
}

func (enc *stringEncoder) AddComplex128(_ string, val complex128) {
	enc.notSupported = true
}

func (enc *stringEncoder) AddDuration(_ string, val time.Duration) {
	enc.AddFloat64("", val.Seconds())
}

func (enc *stringEncoder) AddFloat64(_ string, val float64) {
	switch {
	case math.IsNaN(val):
		enc.value = "NaN"
	case math.IsInf(val, 1):
		enc.value = "+Inf"
	case math.IsInf(val, -1):
		enc.value = "-Inf"
	default:
		enc.value = strconv.FormatFloat(val, 'f', -1, 64)
	}
}

func (enc *stringEncoder) AddInt64(_ string, val int64) {
	enc.value = strconv.FormatInt(val, 10)
}

func (enc *stringEncoder) AddReflected(key string, obj interface{}) error {
	if obj == nil {
		enc.value = "null"
		return nil
	}
	enc.notSupported = true
	return nil
}

func (enc *stringEncoder) AddString(key, val string) {
	enc.value = val
}

func (enc *stringEncoder) AddTime(_ string, val time.Time) {
	enc.value = val.Format(time.RFC3339Nano)
}

func (enc *stringEncoder) AddUint64(_ string, val uint64) {
	enc.value = strconv.FormatUint(val, 10)
}

func (enc *stringEncoder) AddComplex64(k string, v complex64) { enc.AddComplex128(k, complex128(v)) }
func (enc *stringEncoder) AddFloat32(k string, v float32)     { enc.AddFloat64(k, float64(v)) }
func (enc *stringEncoder) AddInt(k string, v int)             { enc.AddInt64(k, int64(v)) }
func (enc *stringEncoder) AddInt32(k string, v int32)         { enc.AddInt64(k, int64(v)) }
func (enc *stringEncoder) AddInt16(k string, v int16)         { enc.AddInt64(k, int64(v)) }
func (enc *stringEncoder) AddInt8(k string, v int8)           { enc.AddInt64(k, int64(v)) }
func (enc *stringEncoder) AddUint(k string, v uint)           { enc.AddUint64(k, uint64(v)) }
func (enc *stringEncoder) AddUint32(k string, v uint32)       { enc.AddUint64(k, uint64(v)) }
func (enc *stringEncoder) AddUint16(k string, v uint16)       { enc.AddUint64(k, uint64(v)) }
func (enc *stringEncoder) AddUint8(k string, v uint8)         { enc.AddUint64(k, uint64(v)) }
func (enc *stringEncoder) AddUintptr(k string, v uintptr)     { enc.AddUint64(k, uint64(v)) }

func (enc *stringEncoder) OpenNamespace(_ string) {
	enc.notSupported = true
}
