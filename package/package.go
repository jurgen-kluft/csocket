package csocket

import (
	cbase "github.com/jurgen-kluft/cbase/package"
	denv "github.com/jurgen-kluft/ccode/denv"
	chash "github.com/jurgen-kluft/chash/package"
	ctime "github.com/jurgen-kluft/ctime/package"
	cunittest "github.com/jurgen-kluft/cunittest/package"
	cuuid "github.com/jurgen-kluft/cuuid/package"
)

const (
	repo_path = "github.com\\jurgen-kluft"
	repo_name = "csocket"
)

func GetPackage() *denv.Package {
	name := repo_name

	// dependencies
	cunittestpkg := cunittest.GetPackage()
	cbasepkg := cbase.GetPackage()
	chashpkg := chash.GetPackage()
	ctimepkg := ctime.GetPackage()
	cuuidpkg := cuuid.GetPackage()

	// main package
	mainpkg := denv.NewPackage(repo_path, repo_name)
	mainpkg.AddPackage(cunittestpkg)
	mainpkg.AddPackage(cbasepkg)
	mainpkg.AddPackage(chashpkg)
	mainpkg.AddPackage(ctimepkg)
	mainpkg.AddPackage(cuuidpkg)

	// main library
	mainlib := denv.SetupCppLibProject(mainpkg, name)
	mainlib.AddDependencies(cbasepkg.GetMainLib()...)
	mainlib.AddDependencies(chashpkg.GetMainLib()...)
	mainlib.AddDependencies(ctimepkg.GetMainLib()...)
	mainlib.AddDependencies(cuuidpkg.GetMainLib()...)

	// test library
	testlib := denv.SetupCppTestLibProject(mainpkg, name)
	mainlib.AddDependencies(cbasepkg.GetTestLib()...)
	mainlib.AddDependencies(chashpkg.GetTestLib()...)
	mainlib.AddDependencies(ctimepkg.GetTestLib()...)
	mainlib.AddDependencies(cuuidpkg.GetTestLib()...)
	testlib.AddDependencies(cunittestpkg.GetTestLib()...)

	// unittest project
	maintest := denv.SetupCppTestProject(mainpkg, name)
	maintest.AddDependencies(cunittestpkg.GetMainLib()...)
	maintest.AddDependency(testlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddTestLib(testlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
