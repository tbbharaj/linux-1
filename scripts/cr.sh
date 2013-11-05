#!/bin/bash
#
# Simple script to generate a code review request for a particular patch for the linux kernel tree
#
# Author: Cristian Gafton <gafton@amazon.com> 2013-06-21
#

# who should see this code review
groups=kaos-kernel,kaos-distro
people=msw,aliguori,gafton,roedel,chegger

set -x

# where is the post-review script located
PR=$(type -p post-review)
if [ -z "$PR" ] ; then
    mydir=$(cd $(dirname $0) && git rev-parse --show-toplevel)
    PR="$(cd ${mydir}/../common/bin && pwd)/post-review"
fi

# args are basically a commit id, a range of commit hashes, and various other
# options that are passed into post-review
hash=
range=
review=
branch=
while [ $# -gt 0 ] ; do
    case $1 in
        *..* ) # we are given a range
            if [ -n "${range}" ] ; then
                echo "ERROR: range ${range} already specified" >&2
                exit -1
            fi
            range=$1
            ;;
        # update an existing code review
        --review=* ) review=${1##--review=} ;;
        --replace=* ) review=${1##--replace=} ;;
        --review-request-id=* ) review=${1##--review-request-id=} ;;
        -r | --review | --replace | --review-request-id )
            shift
            review=$1
            ;;

        -b | --branch )
            shift
            branch=$1
            ;;
        --branch=* ) branch=${1##--branch=} ;;

        * ) # assume it is a single hash
            if [ -n "${hash}" ] ; then
                echo "ERROR: hash ${hash} already specified" >&2
                exit -1
            fi
            hash=$1
            ;;
    esac
    shift
done

# need to know what we'll be posting
if [ -z "${hash}" -a -z "${range}" ] ; then
    echo "ERROR: hash or hash range needed for git repo commits"
    exit -2
fi

# file where we build the description for the commit
description=$(/bin/mktemp /tmp/cr-XXXXXX.txt)
trap "rm -fv ${description}" 0 9 15

# if we are given a single commit ID, use the parent to the commit ID to pull
# a single-commit range.
if [ -z "${range}" ] ; then
    range="${hash}^..${hash}"
    summary=$(git log --format="[AMI kernel] %s" ${hash} -1)
    git log --format="This is a cherry-pick patch from upstream kernel tree.%n%nSubject: %s%nAuthor: %an <%ae>%nDate: %ad%n%n%b" ${hash} -1 >${description}
else
    summary="[AMI kernel] Multiple patches from upstream"
    echo "This is a series of cherry-pick commits from the upstream kernel tree." > ${description}
    echo -e "\n ---- patches included ----" >> ${description}
    git log --format="* %s [%ae]" ${range} --reverse >> ${description}
    echo -e "\n ---- commit logs for patches ---- " >> ${description}
    git log --format="Author: %an <%ae>%nDate: %ad%nSubject: %s%n%n%b%n----------------------------%n" --reverse ${range} >> ${description}
fi

if [ -z "${branch}" ]; then
    branch=$(git rev-parse --abbrev-ref HEAD)
fi

git diff -U1000000 --minimal --no-color ${range} | \
    ${PR} --target-groups=${groups} --target-people=${people} \
    --summary="${summary}" --description-file=${description} \
    --testing-done="It compiled!!!" \
    --branch="${branch}" ${review:+-r ${review}} \
    -a -
rm -f ${description}
