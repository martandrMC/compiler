#!/bin/bash

function build {
	case $1 in
		"release") GCC_ARGS="-Wall -Wextra -pedantic -O2" ;;
		"debug") GCC_ARGS="-Wall -Wextra -pedantic -g" ;;
	esac
	build_rec 'src'
	binfiles=$(find 'bin' -maxdepth 1 -mindepth 1 -type f -name "*.o")
	binfiles=$(echo "$binfiles" | tr '\n' ' ')
	echo "Executable: $binfiles -> bin/compiler"
	gcc $GCC_ARGS -o bin/compiler $binfiles
}

function build_rec {
	local incldir=$(echo "$1" | sed -e 's/src/incl/')
	local bindir=$(echo "$1" | sed -e 's/src/bin/')

	find $1 -maxdepth 1 -mindepth 1 -type f -name "*.c" \
	| while read -r srcfile ; do
		filename=$(echo "$srcfile" | xargs basename | cut -d'.' -f1)
		binfile="$bindir/$filename.o"
		
		mkdir -p "$bindir"
		echo "Building: $srcfile -> $binfile"
		gcc $GCC_ARGS -c -o "$binfile" $srcfile -Iincl -I"$incldir"
	done

	find $1 -maxdepth 1 -mindepth 1 -type d \
	| while read -r srcdir ; do
		build_rec "$srcdir"
		bindir=$(echo "$srcdir" | sed -e 's/src/bin/')
		binfiles=$(find $bindir -maxdepth 1 -mindepth 1 -type f -name "*.o")
		binfiles=$(echo "$binfiles" | tr '\n' ' ')
		echo "Merging: $binfiles -> ${bindir}_dir.o"
		ld --relocatable -o "${bindir}_dir.o" $binfiles
	done
}

function clean {
	[[ -d bin/ ]] && rm -r bin/
}

case $1 in
	"build") build $2 ;;
	"clean") clean ;;
esac
