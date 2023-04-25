#!/bin/bash
# ---------------------------------------------------
# print a list of all sound banks recursively 
#
# Parameters : 
#   path    search path
#   any other arguments (short and long forms) will be handed over to dx7dump
#
# Revision history :
#   28/02/2023, V1.0 - Creation by B.Lex
# ---------------------------------------------------

# dx7dump arguments
dx7_opt=""

# default search-path
searchpath="."	


printbank () {
	# remove "./" from the beginning of the path
	pathname="${1/#\.\//}"
	echo "$pathname"
	dx7dump $dx7_opt "$1"
	echo ""
}

# argument parsing
while [ -n "$1" ]; do
	case "$1" in
		-p) # special case for -p with argument
			dx7_opt+="$1 "
			shift
			dx7_opt+="$1 " 
			;; 
		-*) dx7_opt+="$1 " ;; 
		*) searchpath="$1" ;;
	esac
	shift   # shift arguments to get next option
done


find $searchpath -type f -iname "*.syx" | sort | while read file; do printbank "$file"; done

