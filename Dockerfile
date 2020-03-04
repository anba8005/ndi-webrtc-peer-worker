FROM ubuntu:bionic

RUN sed --in-place --regexp-extended "s/(\/\/)(archive\.ubuntu)/\1lt.\2/" /etc/apt/sources.list && \
    apt update && apt -y install build-essential make nasm sudo cmake libx11-dev libglu1-mesa-dev \
    libavahi-client3 libva-dev

RUN mkdir -p /code/build/CMakeFiles

COPY ./cmake /code/cmake

COPY ./vendor /code/vendor

COPY ./vendor/ndi/lib/x86_64-linux-gnu/* /usr/lib/x86_64-linux-gnu/

COPY ./CMakeLists.txt /code/

COPY ./src /code/src

WORKDIR /code/build

RUN cmake -DCMAKE_BUILD_TYPE=RELEASE .. || cmake -DCMAKE_BUILD_TYPE=RELEASE ..

CMD /usr/bin/make



