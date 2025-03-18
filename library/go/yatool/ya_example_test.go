package yatool_test

import (
	"log"
	"os"
	"os/exec"

	"a.yandex-team.ru/library/go/yatool"
)

func ExampleFindYa() {
	ya, err := yatool.FindYa("arcadia/path")
	if err != nil {
		log.Fatalf("failed to find ya binary: %s\n", err)
	}

	yaCmd := exec.Command(ya, "--help")
	yaCmd.Stdout = os.Stdout
	yaCmd.Stderr = os.Stderr

	if err := yaCmd.Run(); err != nil {
		log.Fatalf("failed to start ya: %s\n", err)
	}
}

func ExampleYa() {
	ya, err := yatool.Ya()
	if err != nil {
		log.Fatalf("failed to find ya binary: %s\n", err)
	}

	yaCmd := exec.Command(ya, "--help")
	yaCmd.Stdout = os.Stdout
	yaCmd.Stderr = os.Stderr

	if err := yaCmd.Run(); err != nil {
		log.Fatalf("failed to start ya: %s\n", err)
	}
}
