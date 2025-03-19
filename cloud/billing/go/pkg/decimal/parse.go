package decimal

import (
	"errors"
	"fmt"
	"io"
	"math/bits"
)

// conversion from string algorithm
func parse(s string) (Decimal128, error) {
	// handle special cases
	// scan doesn't handle ±Inf
	if len(s) == 3 {
		switch s {
		case "Inf", "inf":
			return decimalInf, nil
		case "Nan", "NaN", "nan":
			return decimalNan, nil
		}
	}
	if len(s) == 4 && (s[0] == '+' || s[0] == '-') && (s[1:] == "Inf" || s[1:] == "inf") {
		if s[0] == '-' {
			return decimalInf.Neg(), nil
		}
		return decimalInf, nil
	}

	// Here should be only decimal number. For ease of parsing we use reader.
	result := Decimal128{}
	r := getStringsReader()
	defer putStringsReader(r)
	r.Reset(s)

	if err := result.scan(r); err != nil {
		return decimalNan, ErrParse.Wrap(err)
	}

	// entire string must have been consumed
	if ch, err2 := r.ReadByte(); err2 == nil {
		return decimalNan, ErrParse.Wrap(fmt.Errorf("expected end of string, found %q", ch))
	} else if err2 != io.EOF {
		return decimalNan, err2
	}

	return result, nil
}

// scan decimal representation for its value
func (d *Decimal128) scan(r io.ByteScanner) (err error) {
	// check sign
	if d.neg, err = scanSign(r); err != nil {
		return
	}
	defer d.norm()
	defer func() {
		if err == io.EOF {
			err = nil
		}
	}()

	// allocate buffers for integer and fraction parts of string representation
	const intSize = precisionDecimals - scaleDecimals
	var partsAlloc [precisionDecimals]byte // allocate bytes for parts on stack
	intP := partsAlloc[:0]
	fracP := partsAlloc[intSize:intSize]

	dotFound := false

	// read byte-by-byte
	for err == nil {
		var ch byte
		ch, err = r.ReadByte()
		if err == io.EOF {
			err = nil
			break
		}
		if err != nil {
			return
		}

		switch {
		case ch == '_': // simply skip underscores
		case '0' <= ch && ch <= '9': // Oh! digit found
			if dotFound { // if decimal dot already parsed then it is fraction part
				if len(fracP) < scaleDecimals { // and we skip digits after 15th
					fracP = append(fracP, ch)
				}
				continue
			}
			if len(intP) >= intSize { // if we got too much digits to integer part then number too large
				return ErrRange.Wrap(errors.New("int part of value overflows implementation"))
			}
			intP = append(intP, ch)
		case ch == '.': // We found decimal dot
			if dotFound { // if this is second dot in string then we got not digit
				return ErrSyntax.Wrap(errors.New("character '.' found twice"))
			}
			dotFound = true
		default: // other characters are errors
			return ErrSyntax.Wrap(fmt.Errorf("unexpected character '%q'", string([]byte{ch})))
		}
	}

	//nolint:ST1023
	var buf [precisionDecimals]byte = zerosBuf

	// representation parts has fixed positions in full buffer
	const bufDotPos = precisionDecimals - scaleDecimals
	_ = copy(buf[bufDotPos-len(intP):bufDotPos], intP)
	_ = copy(buf[bufDotPos:bufDotPos+len(fracP)], fracP)

	// at now buf contains full decimal representation and we sure that this is correct base 10 integer
	// constants copied from big.nat.scan for base=10
	const (
		base           = 10
		digsInWord     = 19                   // decimal digits in one uint64
		wordShiftValue = 10000000000000000000 // multiplier for shift value to next word
	)

	var carry uint64
	d.abs = [2]uint64{}

	// collect first word in lower word of decimal
	for _, ch := range buf[:digsInWord] {
		d.abs[0] = d.abs[0]*base + uint64(ch-'0')
	}
	// shift to high word
	d.abs[1], carry = bits.Mul64(d.abs[0], wordShiftValue)

	// collect second word in lower word of decimal
	d.abs[0] = 0
	for _, ch := range buf[digsInWord:] {
		d.abs[0] = d.abs[0]*base + uint64(ch-'0')
	}

	// add lower part from first shift
	d.abs[0], carry = bits.Add64(d.abs[0], carry, 0)
	d.abs[1] += carry

	return nil
}

func scanSign(r io.ByteScanner) (neg bool, err error) {
	var ch byte
	if ch, err = r.ReadByte(); err != nil {
		return
	}
	switch ch {
	case '-':
		neg = true
	case '+':
		// nothing to do
	default:
		err = r.UnreadByte()
	}
	return
}
