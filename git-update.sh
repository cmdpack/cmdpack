#!/bin/bash

git="git@github.com:rufaswan/cmdpack.git"
ups="git@github.com:cmdpack/cmdpack.git"

msg="
please type a comment to commit and push
  type -retry to retry git push [no commit]
  type -pull  to do a git pull
  type -fetch to sync your forked repo
  type -force to overwrite the repo
"
[ $# = 0 ] && { echo "$msg"; exit; }

git remote rm  origin
git remote add origin "$git"
git remote rm  upstream
git remote add upstream "$ups"

case "$1" in
	"-pull")
		echo "git pull $git : master"
		git pull origin master
		;;
	"-force")
		[[ "$2" == "i_really_want_to_do_this" ]] || exit
		echo "git push --force $git : master"
		git push --force origin master
		;;
	"-retry")
		echo "git push $git"
		git push origin master
		;;
	"-fetch")
		echo "git fetch $ups"
		git fetch upstream
		git checkout master
		git merge upstream/master
		;;
	*)
		echo "git push $git : $@"
		git add .
		git ls-files --deleted -z | xargs -0 git rm
		git reflog expire --expire=now --all
		git gc --prune=now
		git commit -m "$@"
		git push origin master
		;;
esac
