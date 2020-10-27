FROM ubuntu:latest
MAINTAINER Lukas Maar

RUN   apt-get update                \
  &&  apt-get -y -q upgrade         \
  &&  apt-get -y -q install         \
      build-essential               \
      git-core                      \
      m4                            \
      scons                         \
      zlib1g                        \
      zlib1g-dev                    \
      libprotobuf-dev               \
      protobuf-compiler             \
      libprotoc-dev                 \
      libgoogle-perftools-dev       \
      swig                          \
      python-dev                    \
      python                        \
      python3-pip                   \
      python-six                   \
      python3-six                   \
  &&  python3 -m pip install        \
      protobuf                      \
  &&  python3 -m pip install        \
      six                           \
  &&  apt-get clean