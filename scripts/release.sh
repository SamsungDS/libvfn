#!/bin/bash

usage() {
    echo "Usage: release.sh VERSION"
    echo ""
    echo "The script does all necessary steps to create a new release."
    echo ""
    echo "Example:"
    echo "  release.sh v2.1.0-rc0     # v2.1.0 release candidate 0"
    echo "  release.sh v2.1.0         # v2.1.0 release"
}

build_doc=true

while getopts "d" o; do
    case "${o}" in
        d)
            build_doc=false
            ;;
        *)
            usage
            ;;
    esac
done
shift $((OPTIND-1))

VERSION=${1:-}

if [ -z "$VERSION" ] ; then
    usage
    exit 1
fi

ver=""

re='^v([0-9]+\.[0-9]+\.[0-9]+)(-rc[0-9]+)?$'
if [[ ! "$VERSION" =~ $re ]]; then
    echo "invalid version string '$VERSION'"
    exit 1
fi

if [[ -n $(git status --porcelain) ]]; then
    echo "tree is dirty"
    exit 1
fi

if [ "$(git rev-parse --abbrev-ref HEAD)" != "main" ] ; then
    echo "not on main branch"
    exit 1
fi

# update meson.build
sed -i -e "0,/[ \t]version: /s/\([ \t]version: \).*/\1\'$ver\',/" meson.build
git add meson.build
git commit -s -m "build: version $VERSION"

git tag -s -m "release $VERSION" "$VERSION"
git push --dry-run origin "$VERSION"^{}:main tag "$VERSION"

read -p "All good? Ready to push changes to remote? [Yy]" -n 1 -r
echo
if [[ $REPLY =~ ^[Yy]$ ]]; then
    git push origin "$VERSION"^{}:main tag "$VERSION"
fi
