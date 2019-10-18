docker run -v $PWD/../src:/code/src -v $PWD/../bin:/code/build/bin \
	-v $PWD/../build:/code/build/CMakeFiles/ndi-webrtc-peer-worker.dir/src \
	--rm -it ndi-worker-build-image-linux:latest $1
