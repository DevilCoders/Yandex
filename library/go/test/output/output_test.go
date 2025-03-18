package output_test

import (
	"fmt"
	"testing"

	"github.com/stretchr/testify/assert"

	"a.yandex-team.ru/library/go/test/output"
)

func TestOutputCapture(t *testing.T) {
	err := output.Replace(output.Stdout)
	assert.NoError(t, err)

	defer output.Reset(output.Stdout)

	fmt.Println("hello")
	captured := output.Catch(output.Stdout)

	assert.Equal(t, "hello\n", string(captured))
}
