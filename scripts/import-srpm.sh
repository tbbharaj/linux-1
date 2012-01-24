#!/bin/bash
#
# Author: Cristian Gafton <gafton@amazon.com>
#
# Simple script to import a fedora src.rpm into the current branch and add the
# Amazon changes to the spec file to get it to build as we need

TOPDIR=$(git rev-parse --show-toplevel)
LINUX_DIR=$TOPDIR/linux

function usage {
    cat <<EOF
Usage : $0 --<series> <src.rpm> <vTAG>
valid options are:
    --fedora13    define and import a fedora13 package
    --fedora14    define and import a fedora14 package
    --fedora15    define and import a fedora15 package
    --fedora16    define and import a fedora16 package
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
        --fedora* )
            SERIES_NAME=fedora
            SERIES_REL=${1##--fedora}
            RPM_PREP=( --define "dist .fc${SERIES_REL}" \
                --define "fedora ${SERIES_REL}" \
                --define "fc${SERIES_REL} 1" )
            ;;
        -* )
            usage
            echo "ERROR: dunno what to do with $1"
            exit -1 ;;
        *.src.rpm ) SRPM=$1 ;;
        * )
            if [ -z "$VTAG" ] ; then
                VTAG=$1
            else
                usage
                echo "ERROR: dunno what to do with $1"
                exit -1
            fi
            ;;
    esac
    shift
done

# get full path for the file since we're going to be cd-ing around
if [ -f $SRPM ] ; then
    SRPM="$(cd $(dirname $SRPM) && pwd -P)/$(basename $SRPM)"
    echo "Importing kernel src.rpm $SRPM"
else
    echo "can't find file: $SRPM"
    usage
    exit -2
fi

if [ -z "$VTAG" ] ; then
    usage
    echo "ERROR: you did not specify the Linux tree vTAG to create a branch at for this import"
    exit -1
else
    pushd $LINUX_DIR >/dev/null
    if ! git show-ref --verify --quiet refs/tags/${VTAG} ; then
        echo "ERROR: invalid vTAG reference: $VTAG does not exist in $LINUX_DIR"
        exit -2
    fi
    popd >/dev/null
fi

if [ -z "$SERIES_NAME" ] ; then
    usage
    echo "ERROR: you did not specify a known --seriesN flag (ie, --fedora14)"
    exit -1
fi

set -e
# grab the tag we're going to use
TAGVER=$(rpm -qp --qf '%{VERSION}-%{RELEASE}' $SRPM 2>/dev/null)
SOURCEDIR=$TOPDIR/${SERIES_NAME}

declare -a rpmopts=(--define "_topdir $TOPDIR" --define "_ntopdir %{_topdir}" --define "_builddir %{_topdir}" \
    --define "_sourcedir ${SOURCEDIR}" --define "_specdir ${SOURCEDIR}" --define "_rpmdir %{_topdir}" \
    --define "_srcrpmdir %{_topdir}" )

echo "Cleaning up work environment..."
#git clean -f -x -d
mkdir -p ${SERIES_NAME}
git ls-files -z ${SERIES_NAME} | egrep -v -zZ '.gitignore' | xargs -r0 git rm --quiet --force

echo "Unpacking source rpm $SRPM..."
rpm  "${rpmopts[@]}" "${RPM_PREP[@]}" -Uvh --quiet $SRPM 2>/dev/null
make upload FILES="*gz *bz2 *xz *tar" SOURCEDIR=${SERIES_NAME}
git add sources

echo "Creating patch script $TOPDIR/scripts/patch..."
sed -e "s#@@LINUX_DIR@@#$LINUX_DIR#" \
    -e "s#@@VTAG@@#$VTAG#" \
    -e "s#@@UPSTREAM@@#${SERIES_NAME}${SERIES_REL}#" \
    -e "s#@@TAGVER@@#${TAGVER}#" \
    <$TOPDIR/scripts/patch.in >$TOPDIR/scripts/patch
chmod 755 $TOPDIR/scripts/patch
sed -i -e "s/local patch=/export patch=/" $SOURCEDIR/kernel.spec

# commit this series to the currenbt branch
git add ${SERIES_NAME}

# this sets up the git tree for the import
function spec_prep_pre {
    cat <<EOF
%__spec_prep_pre %{___build_pre}\\
\\
$TOPDIR/scripts/patch --start\\
%{nil}

EOF
}

# this commits and cleans up the prepped tree
function spec_prep_post {
    cat <<EOF
%__spec_prep_post $TOPDIR/scripts/patch --finish\\
%{___build_post}\\
%{nil}

EOF
}

# now prep the tree
export PATH=$TOPDIR/scripts:$PATH
rpmbuild "${rpmopts[@]}" "${RPM_PREP[@]}" \
    --define "$(spec_prep_post)" \
    -bp --nodeps --target=x86_64 $SOURCEDIR/kernel.spec
git add linux.vers

TAG="${SERIES_NAME}${SERIES_REL}/${TAGVER}"
git commit --allow-empty --quiet -m "imported source rpm ${TAG}"
git tag -m "imported source rpm ${TAG}" --force "${TAG}"

exit 0

MYBRANCH=$(git rev-parse --abbrev-ref HEAD)

pushd $PREPDIR
git ls-files -z | egrep -zZ -v "^.pkginfo" | xargs -r0 git rm --quiet
tar cf - -C $TOPDIR/kernel-2.6.[3-9][0-9]*/linux-2.6.*.i686 . | tar xf -
tar cf - -C $TOPDIR/kernel-2.6.[3-9][0-9]*/linux-2.6.*.x86_64 . | tar xf -
rm -f .config .config.old config-*
git add .
popd

echo "Comitting work...."
pushd $PREPDIR
git commit -m "imported source rpm: $TAG"
git tag -f fedora/$TAG
popd

pushd $TOPDIR
git add prep
git commit -m "imported source rpm: $TAG"
git tag -f fedora/$TAG
popd


echo "Import of $SRPM tagged as fedora/$TAG"
git repack -d && git submodule --quiet foreach git repack -d
