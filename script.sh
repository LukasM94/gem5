#!/usr/bin/env bash

source ./color.sh

DOCKER_IMAGE="gem5"
DOCKER_SRC_DIR="/home/lukas/Programs/gem5/"
DOCKER_FILE_NAME="Dockerfile"
THREADS=5

function print_usage {
  echo "./script.sh"
  echo ""
  echo "exec a shell"
  echo "./script.sh shell"
  echo ""
  echo "builds the gem.opt file"
  echo "./script.sh build <threads>"
  echo "ex. ./script.sh build 5"
  echo ""
  echo "builds the gem.opt file and simulates config file "
  echo "./script.sh run <threads> <config_file>"
  echo "./script.sh run <threads> <config_file> <debug_flags>"
  echo "ex. ./script.sh simulate configs/learning_gem5/simple_cache_obj/run.py"
  echo "ex. ./script.sh simulate configs/learning_gem5/simple_cache_obj/run.py"\
       "SimpleCacheObj"
  echo ""
  echo "simulates config file "
  echo "./script.sh simulate <config_file>"
  echo "./script.sh simulate <config_file> <debug_flags>"
  echo ""
  echo "build only docker image"
  echo "./script.sh docker-build"
  echo ""
  echo "force build docker image"
  echo "./script.sh docker-build-force"
}

function docker_build {
  docker_image_exists
  if [[ "$?" -ne "1" ]]; then
    build_image
    RET="$?"
    if [[ "$RET" -ne "0" ]]; then
      debug_fail "return value was $RET"
      exit -1
    fi;
  fi;
}

function docker_image_exists {
  DOCKER_IMAGE_EXISTS=$(docker images -q "$DOCKER_IMAGE" | wc -l)
  debug_info "YELLOW" "check whether docker image exists"
  if [[ "$DOCKER_IMAGE_EXISTS" -eq "0" ]]
  then
    debug_info "LIGHT_RED" "docker image does not exists"
    return 0
  else
    debug_info "WHITE" "docker image exists"
    return 1
  fi
}

function build_image {
  debug_info "YELLOW" "build docker image"
  if docker build "$DOCKER_SRC_DIR" -t "$DOCKER_IMAGE" ; then
    debug_info "WHITE" "successfully build docker image"
    return 0
  else
    debug_fail "could not build docker image"
    return 1
  fi
}

function shell {
  debug_info "BROWN_ORANGE" "run a shell"
  docker run -u root \
    --mount "type=bind,src=$DOCKER_SRC_DIR,dst=/gem5" \
    --rm \
    -it gem5 \
    "/usr/bin/bash"
  if [[ "$?" -ne "0" ]]; then
    debug_fail "error occured"
  fi
}

function run {
  debug_info "YELLOW" "build docker image"
  docker run -u $UID:$GID \
    --mount "type=bind,src=$DOCKER_SRC_DIR,dst=/gem5" \
    --rm \
    -it gem5 \
    "/gem5/run.sh" \
    "$1 $2 $3 $4"
  if [[ "$?" -ne "0" ]]; then
    debug_fail "error occured"
  fi
}

if [[ "$#" -eq 0 ]]; then
  print_usage
  exit -1
fi
case "$1" in
"docker-build-force"*)
  build_image
  if [[ "$?" -ne "0" ]]; then
    exit -1
  fi
  exit 0
  ;;
"docker-build"*)
  docker_build
  ;;
"shell"*)
  docker_build
  shell
  ;;
"build"*)
  docker_build
  if [[ "$#" -ge 5 ]]; then
    print_usage
    exit -1
  fi
  run "build" $2 $3 $4
  ;;
"run"*)
  docker_build
  if [[ "$#" -ge 5 ]]; then
    print_usage
    exit -1
  fi
  run "run" $2 $3 $4
  ;;
"simulate"*)
  docker_build
  if [[ "$#" -ge 5 ]]; then
    print_usage
    exit -1
  fi
  run "simulate" $2 $3 $4
  ;;
*)
  print_usage
  exit -1
  ;;
esac