package masksecret

// Phone masks string secret representing telephone number
func Phone(s string) string {
	res := []rune(s)

	var masked int
	for i := len(res) - 1; i >= 0; i-- {
		v := res[i]
		if v >= '0' && v <= '9' && masked < 4 {
			v = SecretPlaceholder
			masked++
		}
		res[i] = v
	}

	return string(res)
}
