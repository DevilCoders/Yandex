package inspection

import (
	"reflect"
	"testing"
	"time"
)

func BenchmarkInspect(b *testing.B) {
	type Account struct {
		Login      string
		Password   string
		LastSignIn time.Time
	}

	type User struct {
		Name       string
		Surname    string
		Patronymic string
		Account    Account
		Aliases    []string
	}

	u := User{
		Name:       "Peter",
		Surname:    "Venkman",
		Patronymic: "",
		Account: Account{
			Login:      "pete",
			Password:   "whoyougonnacall",
			LastSignIn: time.Now(),
		},
	}

	b.ReportAllocs()
	b.ResetTimer()
	for i := 0; i < b.N; i++ {
		Inspect(u)
	}
}

func BenchmarkInspected_Reflection(b *testing.B) {
	testCases := []*Inspected{
		Inspect("shimba"),
		Inspect(42),
		Inspect(4.2),
		Inspect(true),
		Inspect(struct{ Name string }{}),
		Inspect([]string{}),
		Inspect(map[int]string{}),
	}

	b.Run("interface", func(b *testing.B) {
		var iface interface{}

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			iface = testCases[i%len(testCases)].Indirect.Interface()
		}

		_ = iface
	})

	b.Run("kind_check", func(b *testing.B) {
		var isString bool

		b.ReportAllocs()
		b.ResetTimer()
		for i := 0; i < b.N; i++ {
			isString = testCases[i%len(testCases)].Indirect.Kind() == reflect.String
		}

		_ = isString
	})
}
