#!/bin/bash

[ $# = 0 ] && exit
git config --global "rufaswan@gmail.com"
git remote add cmd "git@github.com:rufaswan/cmdpack.git"

git add -A .
git commit -m "$1"
git push cmd master
