#!/bin/bash
set -e

find $1 -maxdepth 5 -name "*.[c,h]" -type f | xargs dos2unix
find $1 -maxdepth 5 -name "*.[c,h]" -type f | xargs astyle -A3s4SNwm2M40fpHUjk3n
find $1 -maxdepth 5 -name "*.orig" -type f  | xargs rm -f
