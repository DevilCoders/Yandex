package main

import (
	"fmt"
	"io/ioutil"
	"os"

	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/bootstrap/terraform/control_plane/generation/balancer"
	"a.yandex-team.ru/cloud/mdb/bootstrap/terraform/control_plane/generation/instance"
)

func main() {
	inF := pflag.StringP("input", "i", "", "ycp compute instance list --format json > input.json")
	outF := pflag.StringP("output", "o", "", "Optional output file")
	targetF := pflag.StringP("target", "t", "instance", "What to generate")
	varFileF := pflag.String("var-file", "", "Path to var-file. Used when generating imports")
	envF := pflag.StringP("env", "e", "preprod", "Environment: prod or preprod")
	pflag.Parse()

	if inF == nil {
		fmt.Println("input not specified")
		os.Exit(1)
	}
	in, err := os.Open(*inF)
	if err != nil {
		fmt.Printf("failed to open file %s\n", *inF)
		os.Exit(1)
	}
	out := os.Stdout
	if outF != nil && *outF != "" {
		out, err = os.Create(*outF)
		if err != nil {
			fmt.Printf("failed to create file %s\n", *outF)
			os.Exit(1)
		}
	}
	inContent, err := ioutil.ReadAll(in)
	if err != nil {
		fmt.Printf("failed to read file content %s\n", *inF)
		os.Exit(1)
	}
	switch *targetF {
	case "instance":
		err = instance.Instances(inContent, out)
	case "instance-imports":
		err = instance.InstanceImports(inContent, out, *varFileF)
	case "target-group":
		err = balancer.TargetGroups(inContent, out, *envF)
	case "target-group-imports":
		err = balancer.TargetGroupsImports(inContent, out, *varFileF)
	default:
		fmt.Println("unknown target: ", *targetF)
	}
	if err != nil {
		fmt.Printf("failed to generate %s\n", *targetF)
		os.Exit(1)
	}
}
