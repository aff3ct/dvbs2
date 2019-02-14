#!/bin/bash
set -x

if [[ $# < 3 ]]; then exit 1; fi

cd examples

i=1
for arg in "$@"
do
	if [[ $i == 1 ]]
	then
		example=$arg
		cd $example
	elif [[ $i == 2 ]]
	then
		params=$arg
	else
		build=$arg
		./$build/bin/my_project $params
		rc=$?; if [[ $rc != 0 ]]; then exit $rc; fi
	fi

	i=$((i+1))
done