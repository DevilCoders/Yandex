package masksecret

import "a.yandex-team.ru/library/go/valid"

func CreditCard(s string) (string, error) {
	if err := valid.CreditCard(s); err != nil {
		return "", err
	}

	res := makePlaceholder(SecretPlaceholder, len(s)-6)
	return s[:2] + res + s[len(s)-4:], nil
}
