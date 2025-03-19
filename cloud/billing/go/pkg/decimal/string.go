package decimal

import (
	"errors"
	"fmt"
	"io"
	"strings"
)

// String returns the decimal representation of d.
func (d Decimal128) String() string {
	if sign, val := d.specialString(); val != "" {
		return sign + val
	}

	bld := strings.Builder{}
	i, f := d.reprText()

	if d.neg {
		_ = bld.WriteByte('-')
	}
	_, _ = bld.WriteString(i)
	if f != "" {
		_ = bld.WriteByte('.')
		_, _ = bld.WriteString(f)
	}

	return bld.String()
}

// Format implements fmt.Formatter. It accepts the formats
// 'f', 'F' or 'v'. All formats processed same way.
// Also supported are the most of package fmt's format
// flags, including '+' and ' ' for sign control,
// specification of minimum digits precision, output field
// width, space or zero padding, and '-' for left or right
// justification.
//
func (d Decimal128) Format(s fmt.State, format rune) {

	switch format {
	case 'f', 'F', 'v':
	default:
		_, _ = fmt.Fprintf(s, "%%!%c(decimal.Decimal128=%s)", format, d.String())
		return
	}

	bld := strings.Builder{}
	sign, repr := d.specialString()
	bld.WriteString(repr)
	if repr == "" {
		switch {
		case d.neg:
			sign = "-"
		case s.Flag('+'):
			sign = "+"
		case s.Flag(' '):
			sign = " "
		}
		intP, fracP := d.reprText()
		bld.WriteString(intP)

		fracPad := ""
		if prec, hasPrec := s.Precision(); hasPrec && prec > 0 {
			fracPad = padSting(prec-len(fracP), "0")
			if prec < len(fracP) {
				fracP = fracP[:prec]
			}
		}
		if fracP != "" || fracPad != "" {
			_ = bld.WriteByte('.')
			_, _ = bld.WriteString(fracP)
			_, _ = bld.WriteString(fracPad)
		}
	}
	repr = bld.String()

	padding := 0
	if width, hasWidth := s.Width(); hasWidth && width > len(sign)+len(repr) {
		padding = width - len(sign) - len(repr)
	}

	switch {
	case s.Flag('0') && d.form == finiteDecimal:
		// 0-padding on left
		_, _ = io.WriteString(s, sign)
		_, _ = io.WriteString(s, padSting(padding, "0"))
		_, _ = io.WriteString(s, repr)
	case s.Flag('-'):
		// padding on right
		_, _ = io.WriteString(s, sign)
		_, _ = io.WriteString(s, repr)
		_, _ = io.WriteString(s, padSting(padding, " "))
	default:
		// padding on left
		_, _ = io.WriteString(s, padSting(padding, " "))
		_, _ = io.WriteString(s, sign)
		_, _ = io.WriteString(s, repr)
	}
}

// Scan is a support routine for fmt.Scanner; it sets d to the value of
// the scanned number. It accepts the formats 'f', 's' and 'v'. All formats
// processed same way.
func (d *Decimal128) Scan(s fmt.ScanState, ch rune) (err error) {
	s.SkipSpace()

	switch ch {
	case 'f', 's', 'v':
	default:
		return ErrScan.Wrap(errors.New("invalid verb"))
	}

	if err := d.scan(byteReader{s}); err != nil {
		return ErrScan.Wrap(err)
	}
	return nil
}

// MarshalText implements the encoding.TextMarshaler interface.
func (d Decimal128) MarshalText() ([]byte, error) {
	return []byte(d.String()), nil
}

// UnmarshalText implements the encoding.TextUnmarshaler interface.
func (d *Decimal128) UnmarshalText(text []byte) (err error) {
	*d, err = parse(string(text))
	return
}

func (d Decimal128) specialString() (sign string, val string) {
	switch d.form {
	case nanDecimal:
		return "", "NaN"
	case infDecimal:
		val = "Inf"
		sign = "+"
		if d.neg {
			sign = "-"
		}
		return
	}
	return
}

// reprText get int representation of d.abs like big.Int and add decimal dot in correct position
func (d Decimal128) reprText() (intPart string, fracPart string) {
	i := getInt()
	defer putInt(i)

	i = d.fillBigInt(i)

	var bbuf [precisionDecimals]byte

	buf := i.Abs(i).Append(bbuf[:0], 10)
	if len(buf) > precisionDecimals { // this should not happen but buffer overflowed
		panic(fmt.Sprintf("buffer overflowed during decimal text building: %v", string(buf)))
	}

	intPart = "0"
	if len(buf) > scaleDecimals {
		intSize := len(buf) - scaleDecimals
		intPart = string(buf[:intSize])
		buf = buf[intSize:]
	}

	if leadingZeros := scaleDecimals - len(buf); leadingZeros > 0 {
		fracPart = string(zerosBuf[:leadingZeros])
	}
	fracPart += string(buf)
	return intPart, strings.TrimRight(fracPart, "0")
}

func padSting(rep int, pc string) string {
	if rep <= 0 {
		return ""
	}
	return strings.Repeat(pc, rep)
}

// byteReader is a local wrapper around fmt.ScanState;
// it implements the ByteReader interface.
type byteReader struct {
	fmt.ScanState
}

func (r byteReader) ReadByte() (byte, error) {
	ch, size, err := r.ReadRune()
	if size != 1 && err == nil {
		err = fmt.Errorf("invalid rune %#U", ch)
	}
	return byte(ch), err
}

func (r byteReader) UnreadByte() error {
	return r.UnreadRune()
}
