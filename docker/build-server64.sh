#!/bin/bash

argv=("$@")
if [ -z "${argv[0]}" ]
then
	echo "Usage: $0 <dockeruser/rtcw> <branch name>"
	exit 1
fi

if [ -d "dockerfiles/build64" ]; then
rm -rf dockerfiles/build64
fi
cp -r ../build64 dockerfiles

docker build --build-arg IMAGE="${argv[0]}:${argv[1]}" -t ${argv[0]}-server64:${argv[1]} -f dockerfiles/rtcw-server64 ./dockerfiles
