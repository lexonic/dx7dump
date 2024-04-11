#!/bin/bash
# ---------------------------------------------------
# print a list of all sound banks recursively 
#
# Parameters : 
#   path    search path
#   any other arguments (short and long forms) will be handed over to dx7dump
#
# Revision history :
#   28/02/2023, v1.0 - Creation by B.Lex
#   20/04/2023, added help-screen
#
# License: GPLv3+
# ---------------------------------------------------

# dx7dump arguments
dx7_opt=""

# default search-path
searchpath="."	


printbank () {
	# remove "./" from the beginning of the path
	filename="${1/#\.\//}"
	#echo "$filename"
	dx7dump $dx7_opt "$filename"
	#echo ""
}

# argument parsing
while [ -n "$1" ]; do
	case "$1" in
		-h|--help)	echo -e "dx7dump wrapper: Print lists of all sound banks recursively.\n"
			echo "Usage: $0 [OPTIONS for dx7dump] [PATH]"
			echo "    An argument WITHOUT a leading '-' or '--' is interpreted as the path where the recursive search will start."
			echo -e "    All other arguments are passed to the dx7dump utility:\n"
			dx7dump -o
			exit
			;;
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

