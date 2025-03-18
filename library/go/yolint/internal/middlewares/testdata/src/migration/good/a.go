package good

import "os"

func ErrCheck() {
	os.Open("test") // want `unhandled error`
}
