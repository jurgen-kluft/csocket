package main

import (
	"github.com/jurgen-kluft/xcode"
	"github.com/jurgen-kluft/xsocket/package"
)

func main() {
	xcode.Generate(xsocket.GetPackage())
}
