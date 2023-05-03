package csocket

import (
	"github.com/jurgen-kluft/cbase/package"
	"github.com/jurgen-kluft/ccode/denv"
	"github.com/jurgen-kluft/chash/package"
	"github.com/jurgen-kluft/ctime/package"
	"github.com/jurgen-kluft/cunittest/package"
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
	mainlib := denv.SetupDefaultCppLibProject("csocket", "github.com\\jurgen-kluft\\csocket")
	mainlib.Dependencies = append(mainlib.Dependencies, basepkg.GetMainLib())
	mainlib.Dependencies = append(mainlib.Dependencies, hashpkg.GetMainLib())
	mainlib.Dependencies = append(mainlib.Dependencies, timepkg.GetMainLib())
	mainlib.Dependencies = append(mainlib.Dependencies, uuidpkg.GetMainLib())

	// 'csocket' unittest project
	maintest := denv.SetupDefaultCppTestProject("csocket_test", "github.com\\jurgen-kluft\\csocket")
	maintest.Dependencies = append(maintest.Dependencies, unittestpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, basepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, hashpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, timepkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, uuidpkg.GetMainLib())
	maintest.Dependencies = append(maintest.Dependencies, mainlib)

	mainpkg.AddMainLib(mainlib)
	mainpkg.AddUnittest(maintest)
	return mainpkg
}
