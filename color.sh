#!/usr/bin/env bash

declare -A COLOR=(
  ["BLACK"]='\033[0;30m'
  ["DARK_GRAY"]='\033[1;30m'
  ["RED"]='\033[0;31m'
  ["LIGHT_RED"]='\033[1;31m'
  ["GREEN"]='\033[0;32m'
  ["LIGHT_GREEN"]='\033[1;32m'
  ["BROWN_ORANGE"]='\033[0;33m'
  ["YELLOW"]='\033[1;33m'
  ["BLUE"]='\033[0;34m'
  ["LIGHT_BLUE"]='\033[1;34m'
  ["PURPLE"]='\033[0;35m'
  ["LIGHT_PURPLE"]='\033[1;35m'
  ["CYAN"]='\033[0;36m'
  ["LIGHT_CYAN"]='\033[1;36m'
  ["LIGHT_GRAY"]='\033[0;37m'
  ["WHITE"]='\033[1;37m'
)
NO_COLOR='\033[0m'

function debug {
  COL=${COLOR[$1]}
  if [[ ! -z "$COL" ]]; then
    printf "${COL}$2${NO_COLOR}\n"
  else
    printf "$2\n"
  fi
}

function debug_info {
  debug $1 "[INFO] $2"
}

function debug_fail {
  debug "RED" "[FAIL] $1"
}
