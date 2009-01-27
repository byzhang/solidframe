#!/bin/bash

function make_cmake_list(){
	rm CMakeLists.txt
	if [ $1 = "*" ]; then
		echo "No child folder found!"
	else
		for name in $*
			do
			if [ $name = "CMakeLists.txt" ]; then
				echo "Skip CMakeLists.txt"
			else
				echo "Adding dir: " $name
				echo -ne "add_subdirectory("$name")\n" >> CMakeLists.txt
			fi
			done
	fi
}

function print_usage(){
	echo "Usage:"
	echo -ne "\n./build.sh [kdevelop] build_type [\"-DXXXX[ -D...]\"]\n\n"
	echo -ne "Where build type can be:\n"
	echo -ne "\tdebug - build uses full debug infos (-g3) and the debug log is activated\n"
	echo -ne "\tmaintain - same as debug but with compilation warnings activated\n"
	echo -ne "\tnolog - full debug info but logs are deactivated\n"
	echo -ne "\trelease - full optimization (-O3)\n"
	echo -ne "\textern - build the tar.gz with the extern libs\n"
	echo -ne "\tdocumentation_full - full API documentation including pdf\n"
	echo -ne "\tdocumentation_fast - fast API documentation\n"
	echo -ne "\nWhen used kdevelop, a kdevelop project will be created else make based project will be created.\n"
	echo -ne "\nExtra framework wide compiler definitions can be specified after 'build_type'. E.g.:\n"
	echo -ne "\t./build.sh kdevelop debug \"-DUINDEX32 -DUINLINES\"\n\n"
	exit
}

if [ "$1" = "" ] ; then
	print_usage
fi

case "$1" in
	"kdevelop")
		if [ "$2" = "" ] ; then
			print_usage
		else
			echo "Building KDevelop project ..."
			cd application
			make_cmake_list *
			cd ../
			
			cd library
			make_cmake_list *
			cd ../
			
			mkdir build
			mkdir "build/$1"
			cd "build/$1"
			cmake -G KDevelop3 -DCMAKE_BUILD_TYPE=$2 -DUDEFS="$3" ../../
			echo "Done building KDevelop project"
		fi
		;;
	"documentation_full")
		echo "Building full documentation ..."
		rm -rf documentation/html/
		rm -rf documentation/latex/
		doxygen
		tar -cjf documentation/full.tar.bz2 documentation/html/ documentation/index.html
		echo "Done building full documentation"
		;;
	"documentation_fast")
		echo "Building documentation..."
		rm -rf documentation/html/
		doxygen Doxyfile.fast
		tar -cjf documentation/fast.tar.bz2 documentation/html/ documentation/index.html
		echo "Done building documentation"
		;;
	"extern")
		echo "Building extern archive"
		cd extern
		tar -cjf solidground_extern_linux.tar.bz2 linux
		echo "Done building extern archive"
		;;
	*)
		THEFLD="$2"
		echo $THEFLD
		if [ "$THEFLD" = "" ] ; then
			THEFLD="$1"
		fi
		cd application
		make_cmake_list *
		cd ../
		
		cd library
		make_cmake_list *
		cd ../
		echo "Preparing build/$THEFLD for $1 build type with \"$3\" parameters..."
		mkdir build
		mkdir "build/$THEFLD"
		cd "build/$THEFLD"
		cmake -DCMAKE_BUILD_TYPE=$1 -DUDEFS="$3" ../../
		echo "Done preparing build/$THEFLD for $1 build type with \"$3\" parameters"
		;;
esac