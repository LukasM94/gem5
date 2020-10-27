#!/usr/bin/env bash

function build {
  debug_info "BLUE" "scons build/X86/gem5.opt -j$THREADS"
  scons build/X86/gem5.opt -j$THREADS
}

function simulate {
  if [[ -z $DEBUG ]]; then
    debug_info "YELLOW" "build/X86/gem5.opt $EXE"
    build/X86/gem5.opt $EXE
  else
    debug_info "YELLOW" "build/X86/gem5.opt --debug-flags=$DEBUG $EXE"
    build/X86/gem5.opt --debug-flags=$DEBUG $EXE
  fi  
}

cd /gem5
source /gem5/color.sh
debug_info "GREEN" "./run.sh $1"
STRING_ARRAY=($1)
FUNCTION=${STRING_ARRAY[0]}

case $FUNCTION in
"build"*)
  THREADS="${STRING_ARRAY[1]}"
  build
  ;;
"run"*)
  THREADS="${STRING_ARRAY[1]}"
  EXE="${STRING_ARRAY[2]}"
  DEBUG="${STRING_ARRAY[3]}"
  build && simulate
  ;;
"simulate"*)
  EXE="${STRING_ARRAY[1]}"
  DEBUG="${STRING_ARRAY[2]}"
  simulate
  ;;
*)
  echo "[FAIL] false function"
  exit -1
  ;;
esac

