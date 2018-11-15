package main

import (
	"github.com/jurgen-kluft/xcode"
	"github.com/jurgen-kluft/xsocket/package"
)

func main() {
	xcode.Init()
	xcode.Generate(xsocket.GetPackage())
}
