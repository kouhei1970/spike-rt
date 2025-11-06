#!/bin/bash
if [ "$1" = "" ]; then
	echo "no argument"
	exit 1
fi

appname=$1

if [ -e "workspace/${appname}" ]; then
	echo "${appname} already exists."
	exit 1
else
	mkdir "workspace/${appname}"
	cd workspace
	../tools/python/bin/python3 ../new_scripts/create.py $appname
	chmod -R 777 ../workspace/$appname/
fi
