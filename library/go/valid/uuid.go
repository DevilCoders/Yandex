package valid

// UUID check if the string is a canonical UUID (version 3, 4 or 5).
func UUID(s string) error {
	if len(s) != 36 {
		return ErrInvalidStringLength
	}

	if s[8] != '-' || s[13] != '-' || s[18] != '-' || s[23] != '-' {
		return ErrInvalidCharsSequence
	}

	for _, c := range s {
		if (c < 'a' || c > 'f') && (c < '0' || c > '9') && c != '-' {
			return ErrInvalidCharacters
		}
	}

	return nil
}

// UUIDv3 check if the string is a canonical UUID version 3.
func UUIDv3(s string) error {
	if err := UUID(s); err != nil {
		return err
	}

	if s[14] != '3' {
		return ErrInvalidCharsSequence
	}

	return nil
}

// UUIDv4 check if the string is a canonical UUID version 4.
func UUIDv4(s string) error {
	if err := UUID(s); err != nil {
		return err
	}

	if s[14] != '4' || (s[19] != '8' && s[19] != '9' && s[19] != 'a' && s[19] != 'b') {
		return ErrInvalidCharsSequence
	}

	return nil
}

// UUIDv5 check if the string is a canonical UUID version 5.
func UUIDv5(s string) error {
	if err := UUID(s); err != nil {
		return err
	}

	if s[14] != '5' || (s[19] != '8' && s[19] != '9' && s[19] != 'a' && s[19] != 'b') {
		return ErrInvalidCharsSequence
	}

	return nil
}
