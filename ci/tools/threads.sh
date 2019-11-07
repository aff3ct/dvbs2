#!/bin/bash
# set -x
if [ -z "$THREADS" ]; then
	if [ ! -f /proc/cpuinfo ]; then
		export THREADS=1
	else
		MAX_THREADS=$(grep -c ^processor /proc/cpuinfo)
		THREADS=$(($MAX_THREADS / 2))
		export THREADS
	fi
else
	if [ ! -f /proc/cpuinfo ]; then
		export THREADS=1
	else
		MAX_THREADS=$(grep -c ^processor /proc/cpuinfo)
		HALF_MAX_THREADS=$(($MAX_THREADS / 2)) # reduce to half the cores because dvbs2o tends to make system memory overflow
		if [ "$THREADS" -gt "$HALF_MAX_THREADS" ]; then
			THREADS=$HALF_MAX_THREADS
		fi
		export THREADS
	fi
fi
