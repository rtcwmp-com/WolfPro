#!/bin/bash

argv=("$@")
if [ -z "${argv[0]}" ]
then
	echo "Usage: $0 <docker git image: <docker username/rtcw> <branch name>"
	exit 1
fi

RTCW_SRC=$(dirname `pwd`)


# build the image that will do the compiling
docker build \
--build-arg BRANCH="${argv[1]}" \
-t ${argv[0]}:${argv[1]} \
-f dockerfiles/rtcw-compile64 ./dockerfiles

# build the files from the local volume inside the container
#  artifacts go to <repo>/build/
docker run -it \
-v $RTCW_SRC:/home/compile/code \
--workdir /home/compile/code/src \
${argv[0]}:${argv[1]} \
make all

mv $RTCW_SRC/build64/wolfpro/wolfpro_bin.pk3 $RTCW_SRC/build64/wolfpro/wolfpro_bin-$(date +%Y%m%d).pk3

bash build-server64.sh ${argv[0]} ${argv[1]}