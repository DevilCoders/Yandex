package bad

import "os"

func ErrCheck() {
	os.Open("test")
}
