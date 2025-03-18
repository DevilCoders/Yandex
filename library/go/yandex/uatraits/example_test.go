package uatraits_test

import (
	"fmt"

	"a.yandex-team.ru/library/go/yandex/uatraits"
)

func ExampleNewDetector() {
	// Create detector with fresh uatraits-data from metrika/uatraits/data/
	detector, err := uatraits.NewDetector()
	if err != nil {
		panic(err)
	}

	// ...or create detector with your custom definitions or cache size
	_, err = uatraits.NewDetector(
		uatraits.Browsers("browser.xml"),
		uatraits.Extras("extras.xml"),
		uatraits.Profiles("profiles.xml"),
		uatraits.CacheSize(500),
	)
	if err != nil {
		panic(err)
	}

	// Detect single user-agent string
	traits := detector.Detect("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/84.0.4147.105 Safari/537.36")

	fmt.Println(traits["BrowserName"])
	fmt.Println(traits.Get("BrowserName"))
	fmt.Println(traits.BrowserName())
	// Output: Chrome
}
