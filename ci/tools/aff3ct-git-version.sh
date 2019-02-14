#!/bin/bash
# set -x

cd lib/aff3ct/

AFF3CT_GIT_VERSION=$(git describe)

if [ ! -z "$AFF3CT_GIT_VERSION" ]
then
	AFF3CT_GIT_VERSION=$(echo $AFF3CT_GIT_VERSION | cut -d $'v' -f2-)
	export AFF3CT_GIT_VERSION
fi

cd ../../