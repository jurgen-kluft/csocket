package main

import (
	"github.com/jurgen-kluft/ccode"
	"github.com/jurgen-kluft/csocket/package"
)

func main() {
	ccode.Init()
	ccode.Generate(csocket.GetPackage())
}
