package csocket

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	denv "github.com/jurgen-kluft/ccode/denv"
	chash "github.com/jurgen-kluft/chash/package"
	ctime "github.com/jurgen-kluft/ctime/package"
	cunittest "github.com/jurgen-kluft/cunittest/package"
	cuuid "github.com/jurgen-kluft/cuuid/package"
)

// GetPackage returns the package object of 'csocket'
func GetPackage() *denv.Package {
	// Dependencies
	unittestpkg := cunittest.GetPackage()
	basepkg := cbase.GetPackage()
	hashpkg := chash.GetPackage()
	timepkg := ctime.GetPackage()
	uuidpkg := cuuid.GetPackage()

	// The main (csocket) package
	mainpkg := denv.NewPackage("csocket")
	mainpkg.AddPackage(unittestpkg)
	mainpkg.AddPackage(basepkg)
	mainpkg.AddPackage(hashpkg)
	mainpkg.AddPackage(timepkg)
	mainpkg.AddPackage(uuidpkg)

	// 'csocket' library
	mainlib := denv.SetupCppLibProject("csocket", "github.com\\jurgen-kluft\\csocket")
	mainlib.AddDependencies(basepkg.GetMainLib()...)
	mainlib.AddDependencies(hashpkg.GetMainLib()...)
	mainlib.AddDependencies(timepkg.GetMainLib()...)
	mainlib.AddDependencies(uuidpkg.GetMainLib()...)

	// 'csocket' unittest project
	maintest := denv.SetupDefaultCppTestProject("csocket_test", "github.com\\jurgen-kluft\\csocket")
	maintest.AddDependencies(unittestpkg.GetMainLib()...)
	maintest.AddDependencies(basepkg.GetMainLib()...)
	maintest.AddDependencies(hashpkg.GetMainLib()...)
	maintest.AddDependencies(timepkg.GetMainLib()...)
	maintest.AddDependencies(uuidpkg.GetMainLib()...)
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
