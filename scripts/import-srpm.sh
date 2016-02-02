#!/bin/bash
#
# Author: Cristian Gafton <gafton@amazon.com>
#
# Simple script to import a fedora src.rpm into the current branch and add the
# Amazon changes to the spec file to get it to build as we need

TOPDIR=$(git rev-parse --show-toplevel)
LINUX_DIR=$TOPDIR/linux

# color in the output helps a lot with parsing
_grn="$(echo -en '\033[1;32m')"  # green
_red="$(echo -en '\033[1;31m')"  # red
_yel="$(echo -en '\033[1;33m')"  # yellow
_def="$(echo -en '\033[0;39m')"  # normal
say_red()    { echo -e "${_red}${1:-}${_def}" ; }
say_green()  { echo -e "${_grn}${1:-}${_def}" ; }
say_yellow() { echo -e "${_yel}${1:-}${_def}" ; }

function usage {
    cat <<EOF
Usage : $0 --<series> <src.rpm> <vTAG>
valid options are:
    --fedoraREL    define and import a Fedora REL package
                   currently REL in [13..29]
* vTAG is a linux kernel tree version release base tag for the package being
  imported, ie, v2.6.39
EOF
}

SRPM=
SERIES_NAME=
SERIES_REL=
VTAG=
# rpm defines for tree running the %prep stage
declare -a RPM_PREP=(--define 'dist linux')
while [ $# -gt 0 ] ; do
    case $1 in
        -h | --help ) usage ; exit 0 ;;
        --fedora1[3-9] | --fedora2[0-9] )
            SERIES_NAME=fedora
            SERIES_REL=${1##--fedora}
            RPM_PREP=( --define "dist .fc${SERIES_REL}" \
                --define "fedora ${SERIES_REL}" \
                --define "fc${SERIES_REL} 1" )
            case $SERIES_REL in
                16 | 17 ) RPM_PREP=("${RPM_PREP[@]}" --without backports) ;;
            esac
            ;;
        -* )
            usage
            say_red "ERROR: dunno what to do with $1"
            exit -1 ;;
        *.src.rpm ) SRPM=$1 ;;
        * )
            if [ -z "$VTAG" ] ; then
                VTAG=$1
            else
                usage
                say_red "ERROR: dunno what to do with $1"
                exit -1
            fi
            ;;
    esac
    shift
done

# get full path for the file since we're going to be cd-ing around
if [ -f $SRPM ] ; then
    SRPM="$(cd $(dirname $SRPM) && pwd -P)/$(basename $SRPM)"
    say_green "Importing kernel src.rpm $SRPM"
else
    say_red "can't find file: $SRPM"
    usage
    exit -2
fi

if [ -z "$VTAG" ] ; then
    usage
    say_red "ERROR: you did not specify the Linux tree vTAG to create a branch at for this import"
    exit -1
else
    pushd $LINUX_DIR >/dev/null
    if ! git show-ref --verify --quiet refs/tags/${VTAG} ; then
        say_red "ERROR: invalid vTAG reference: $VTAG does not exist in $LINUX_DIR"
        exit -2
    fi
    popd >/dev/null
fi

if [ -z "$SERIES_NAME" ] ; then
    usage
    say_red "ERROR: you did not specify a known --seriesN flag (ie, --fedora14)"
    exit -1
fi

trap 'say_red "script exited with ERROR: $BASH_COMMAND"' ERR
set -e -x

# grab the tag we're going to use
TAGVER=$(rpm -qp --qf '%{VERSION}-%{RELEASE}' $SRPM 2>/dev/null)
SOURCEDIR=$TOPDIR/${SERIES_NAME}
TAG="${SERIES_NAME}${SERIES_REL}/${TAGVER}"

declare -a rpmopts=(--define "_topdir $TOPDIR" --define "_ntopdir %{_topdir}" --define "_builddir %{_topdir}" \
    --define "_sourcedir ${SOURCEDIR}" --define "_specdir ${SOURCEDIR}" --define "_rpmdir %{_topdir}" \
    --define "_srcrpmdir %{_topdir}" )

say_yellow "Cleaning up work environment..."
#git clean -f -x -d
mkdir -p ${SERIES_NAME}
cat >>${SERIES_NAME}/.gitignore <<EOF
*~
*gz
*bz2
*xz
*.tar
*.bin
*.orig
*.rej
EOF
sort -u -o srpm/.gitignore srpm/.gitignore
if [ -f ${SERIES_NAME}/.gitignore ] ; then sort -u -o ${SERIES_NAME}/.gitignore ${SERIES_NAME}/.gitignore ; fi
git ls-files -z ${SERIES_NAME} | egrep -v -zZ '.gitignore' | xargs -r0 git rm --quiet --force
mkdir -p ${SERIES_NAME} srpm

say_yellow "Unpacking source rpm $SRPM..."
rpm  "${rpmopts[@]}" -Uvh --quiet $SRPM 2>/dev/null

say_yellow "Creating patch script $TOPDIR/scripts/patch..."
sed -e "s#@@LINUX_DIR@@#$LINUX_DIR#" \
    -e "s#@@VTAG@@#$VTAG#" \
    -e "s#@@UPSTREAM@@#${SERIES_NAME}${SERIES_REL}#" \
    -e "s#@@TAGVER@@#${TAGVER}#" \
    <$TOPDIR/scripts/patch.in >$TOPDIR/scripts/patch
chmod 755 $TOPDIR/scripts/patch
SPECFILE=$(ls $SOURCEDIR/kernel*.spec|head -1)
sed -i -e "s/local patch=/export patch=/" $SPECFILE

# commit this series to the currenbt branch
git add ${SERIES_NAME}

# this sets up the git tree for the import
function spec_prep_pre {
    cat <<EOF
__spec_prep_pre %{___build_pre}\\
\\
$TOPDIR/scripts/patch --start\\
%{nil}

EOF
}

# this commits and cleans up the prepped tree
function spec_prep_post {
    cat <<EOF
__spec_prep_post cd %{_builddir}/%{?buildsubdir:%{buildsubdir}}%{?!buildsubdir:%{name}-%{version}}\\
$TOPDIR/scripts/patch --finish\\
%{___build_post}\\
%{nil}

EOF
}

# now prep the tree
export PATH=$TOPDIR/scripts:$PATH
rpmbuild "${rpmopts[@]}" "${RPM_PREP[@]}" \
    --define "$(spec_prep_post)" \
    -bp --nodeps --target=x86_64 $SPECFILE

git add linux.vers
git commit --allow-empty --quiet -m "imported source rpm ${TAG}"
say_green "Import of $SRPM completed"
