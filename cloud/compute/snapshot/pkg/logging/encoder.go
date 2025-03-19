package logging

import (
	"bytes"
	"unicode"

	"go.uber.org/zap/buffer"
	"go.uber.org/zap/zapcore"
)

var bufferPool = buffer.NewPool()

func hasSpace(s string) bool {
	for _, r := range s {
		if unicode.IsSpace(r) {
			return true
		}
	}
	return false
}

// OneLetterLevelEncoder returns the first capital letter of level.
func OneLetterLevelEncoder(l zapcore.Level, enc zapcore.PrimitiveArrayEncoder) {
	enc.AppendString(l.CapitalString()[:1])
}

// OneLetterColonLevelEncoder returns the first capital letter of level plus colon.
func OneLetterColonLevelEncoder(l zapcore.Level, enc zapcore.PrimitiveArrayEncoder) {
	enc.AppendString(l.CapitalString()[:1] + ":")
}

// MetaKey is a specification of field which needs special handling.
type MetaKey struct {
	// Original name.
	Name string
	// Name for [name=value] notation.
	ShortName string
	// Variable name for GetMetaFields.
	VariableName string
}

// Encoder is a SSKV-like logging encoder.
type Encoder struct {
	*zapcore.EncoderConfig
	buf    *buffer.Buffer
	fields []zapcore.Field

	// Prefix fields
	metaKeys   []MetaKey
	metaKeyMap map[string]int
}

// NewEncoder returns a new Encoder instance.
func NewEncoder(encCfg zapcore.EncoderConfig, metaKeys []MetaKey) *Encoder {
	metaKeyMap := make(map[string]int, len(metaKeys))
	for i, meta := range metaKeys {
		metaKeyMap[meta.Name] = i
	}

	return &Encoder{
		EncoderConfig: &encCfg,
		buf:           bufferPool.Get(),
		metaKeys:      metaKeys,
		metaKeyMap:    metaKeyMap,
	}
}

// GetMetaFields returns a map of variables valid for journald.
func (enc *Encoder) GetMetaFields(ent zapcore.Entry, fields []zapcore.Field) map[string]string {
	result := make(map[string]string)

	clone := enc.Clone()
	clone.Add(fields)

	for _, field := range clone.fields {
		if index, ok := enc.metaKeyMap[field.Key]; ok {
			result[enc.metaKeys[index].VariableName] = enc.getUnquotedValue(field)
		}
	}

	return result
}

// Clone returns a copy of encoder.
func (enc *Encoder) Clone() *Encoder {
	clone := enc.clone()
	_, _ = clone.buf.Write(enc.buf.Bytes())
	return clone
}

func (enc *Encoder) clone() *Encoder {
	clone := &Encoder{}
	clone.EncoderConfig = enc.EncoderConfig
	clone.buf = bufferPool.Get()
	clone.fields = append(clone.fields, enc.fields...)
	clone.metaKeys = enc.metaKeys
	clone.metaKeyMap = enc.metaKeyMap
	return clone
}

// EncodeEntry builds a data buffer from entry.
func (enc *Encoder) EncodeEntry(ent zapcore.Entry, fields []zapcore.Field) (*buffer.Buffer, error) {
	line := bufferPool.Get()

	tmpEntry := ent
	tmpEntry.Message = ""
	buf, err := zapcore.NewConsoleEncoder(*enc.EncoderConfig).EncodeEntry(tmpEntry, nil)
	if err != nil {
		return nil, err
	}
	bts := buf.Bytes()
	// ConsoleEncoder separates fields with \t which is too wide.
	bts = bytes.Replace(bts, []byte{'\t'}, []byte{' '}, -1)
	if len(bts) >= 2 {
		_, _ = line.Write(bts[:len(bts)-2])
	} else {
		// Must not happen.
		_, _ = line.Write(bts)
	}
	line.AppendByte(' ')

	// Add fields that are treated as meta.
	l := line.Len()
	err = enc.writeMetaFields(line, fields)
	if err != nil {
		return nil, err
	}
	if line.Len() != l {
		line.AppendByte(' ')
	}

	// Add the message itself.
	if enc.MessageKey != "" {
		line.AppendString(ent.Message)
		line.AppendByte('\t')
	}

	// Add any structured context.
	err = enc.writeNormalFields(line, fields)
	if err != nil {
		return nil, err
	}

	// If there's no stacktrace key, honor that; this allows users to force
	// single-line output.
	if ent.Stack != "" && enc.StacktraceKey != "" {
		line.AppendByte('\n')
		line.AppendString(ent.Stack)
	}

	if enc.LineEnding != "" {
		line.AppendString(enc.LineEnding)
	} else {
		line.AppendString(zapcore.DefaultLineEnding)
	}
	return line, nil
}

// Add adds additional fields to encoder.
func (enc *Encoder) Add(fields []zapcore.Field) {
	enc.fields = append(enc.fields, fields...)
}

func (enc *Encoder) addElementSeparator() {
	last := enc.buf.Len() - 1
	if last < 0 {
		return
	}

	switch enc.buf.Bytes()[last] {
	case '=', '[':
		return
	default:
		enc.buf.AppendByte(' ')
	}
}

func (enc *Encoder) getEmptyConfig() zapcore.EncoderConfig {
	return zapcore.EncoderConfig{
		LineEnding:     "$",
		EncodeTime:     zapcore.ISO8601TimeEncoder,
		EncodeDuration: zapcore.StringDurationEncoder,
	}
}

func (enc *Encoder) appendField(field zapcore.Field) error {
	if field.Type == zapcore.SkipType {
		return nil
	}

	enc.addElementSeparator()

	buf, err := enc.getByteStringValue(zapcore.Field{Type: zapcore.StringType, String: field.Key})
	if err != nil {
		return err
	}
	_, err = enc.buf.Write(buf)
	if err != nil {
		return err
	}

	enc.buf.AppendByte('=')

	field.Key = ""
	buf, err = enc.getByteStringValue(field)
	if err != nil {
		return err
	}
	_, err = enc.buf.Write(buf)
	return err
}

func (enc *Encoder) getByteStringValue(field zapcore.Field) ([]byte, error) {
	buf, err := zapcore.NewJSONEncoder(enc.getEmptyConfig()).EncodeEntry(
		zapcore.Entry{}, []zapcore.Field{field})
	if err != nil {
		return nil, err
	}
	bts := buf.Bytes()
	if (field.Type == zapcore.StringType ||
		field.Type == zapcore.ByteStringType ||
		field.Type == zapcore.StringerType) &&
		!hasSpace(buf.String()) && len(bts) >= 8 {
		return bts[5 : len(bts)-3], nil
	} else if len(bts) >= 6 {
		return bts[4 : len(bts)-2], nil
	}
	// Best-effort, must not happen.
	return bts, nil
}

func (enc *Encoder) getUnquotedValue(field zapcore.Field) string {
	field.Key = ""
	buf, _ := zapcore.NewJSONEncoder(enc.getEmptyConfig()).EncodeEntry(
		zapcore.Entry{}, []zapcore.Field{field})
	s := buf.String()
	if (field.Type == zapcore.StringType ||
		field.Type == zapcore.ByteStringType ||
		field.Type == zapcore.StringerType) && len(s) >= 8 {
		return s[5 : len(s)-3]
	} else if len(s) >= 6 {
		return s[4 : len(s)-2]
	}
	// Must not happen.
	return s
}

func (enc *Encoder) writeMetaFields(line *buffer.Buffer, fields []zapcore.Field) error {
	clone := enc.Clone()
	clone.buf = line
	clone.Add(fields)

	fieldMap := make(map[string]zapcore.Field, len(clone.fields))
	for _, field := range clone.fields {
		fieldMap[field.Key] = field
	}

	// We want to output fields in order of metaKeys.
	for _, meta := range clone.metaKeys {
		if field, ok := fieldMap[meta.Name]; ok {
			field.Key = meta.ShortName
			clone.buf.AppendByte('[')
			err := clone.appendField(field)
			if err != nil {
				return err
			}
			clone.buf.AppendByte(']')
			clone.buf.AppendByte(' ')
		}
	}

	return nil
}

func (enc *Encoder) writeNormalFields(line *buffer.Buffer, fields []zapcore.Field) error {
	clone := enc.Clone()
	clone.buf = line
	clone.Add(fields)

	for _, field := range clone.fields {
		if _, ok := enc.metaKeyMap[field.Key]; !ok {
			err := clone.appendField(field)
			if err != nil {
				return err
			}
		}
	}
	return nil
}
