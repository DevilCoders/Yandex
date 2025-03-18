package main

import (
	"crypto/rand"

	"github.com/gofrs/uuid"
)

func randomString(letters string, n int) string {
	suffix := make([]byte, n)
	generateRandom(suffix)
	for i := range suffix {
		suffix[i] = byte(letters[int(suffix[i])%len(letters)])
	}
	return string(suffix)
}

func GenerateClientServerKeys() (string, string) {
	prefix := randomString(captchaKeysValidIDRunes, 20)
	return prefix + randomString(captchaKeysValidIDRunes, 20), prefix + randomString(captchaKeysValidIDRunes, 20)
}

const (
	cloudIDGenPartLen       = 17
	cloudValidIDRunes       = "abcdefghijklmnopqrstuv0123456789"
	captchaKeysValidIDRunes = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
)

func generateRandom(buffer []byte) {
	_, err := rand.Read(buffer)
	if err != nil {
		panic(err)
	}
}

func generateSuffix() string {
	return randomString(cloudValidIDRunes, cloudIDGenPartLen)
}

func GenerateCaptchaID(prefix string) string {
	suffix := generateSuffix()
	return prefix + suffix
}

func GenerateOperationID(prefix string) string {
	return GenerateCaptchaID(prefix)
}

func GenerateUUID() (string, error) {
	u, err := uuid.NewV4()
	if err != nil {
		return "", err
	}
	return u.String(), nil
}

func GenerateUUIDSafe() string {
	u, err := uuid.NewV4()
	if err != nil {
		return ""
	}
	return u.String()
}
