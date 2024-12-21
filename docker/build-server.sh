#!/bin/bash

argv=("$@")
if [ -z "${argv[0]}" ]
then
	echo "Usage: $0 <dockeruser/rtcw> <branch name>"
	exit 1
fi

if [ ! -d "dockerfiles/build" ]; then
rm -rf dockerfiles/build
fi
cp -r ../build dockerfiles

docker build --build-arg IMAGE="${argv[0]}:${argv[1]}" -t ${argv[0]}-server:${argv[1]} -f dockerfiles/rtcw-server ./dockerfiles
