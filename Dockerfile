FROM ryjen/cpp-coveralls

ARG CMAKE_DEFINES

RUN apk update

RUN apk add \
    fts-dev \
    libarchive-dev \
    git \
    cmake \
    autoconf \
    automake

COPY . /usr/src

RUN mkdir -p /usr/src/docker-build

WORKDIR /usr/src/docker-build

RUN cmake ${CMAKE_DEFINES} ..

RUN make

CMD "make", "test", "ARGS=-V"

