# adapted by Eric Schulte from clang-svn         -*- shell-script -*-
#
# Like clang-svn but includes all of the headers etc... you'll need if
# you want to compile tools built upon clang.
#
# differences are:
# - simpler build
# - fuller build
# - installs headers
# - installs clang-tools-extra
# - builds with make rather than cmake
# - pollutes source directory (but who cares)
#
# Maintainer: Eric Schulte <schulte.eric@gmail.com>
# Contributor: Sven-Hendrik Haase <sh@lutzhaase.com>
# Contributor: Jan "heftig" Steffens <jan.steffens@gmail.com>
# Contributor: Evangelos Foutras <foutrelis@gmail.com>
# Contributor: Sebastian Nowicki <sebnow@gmail.com>
# Contributor: Devin Cofer <ranguvar{AT]archlinux[DOT}us>
# Contributor: Tobias Kieslich <tobias@justdreams.de>
# Contributor: Geoffroy Carrier <geoffroy.carrier@aur.archlinux.org>
# Contributor: Tomas Lindquist Olsen <tomas@famolsen.dk>
# Contributor: Roberto Alsina <ralsina@kde.org>
# Contributor: Gerardo Exequiel Pozzi <vmlinuz386@yahoo.com.ar>

pkgname=clang-dev-svn
pkgver=166623
pkgrel=1
pkgdesc="Low Level Virtual Machine with Clang from SVN"
arch=('i686' 'x86_64')
url="http://llvm.org/"
license=('custom:University of Illinois/NCSA Open Source License')
depends=('gcc-libs' 'libffi' 'python2' "gcc>=4.7.0")
makedepends=('svn')
provides=('clang' 'llvm')
conflicts=(llvm llvm-svn llvm-ocaml clang clang-svn clang-analyzer)

_svnmod="llvm"

_checkout() {
    local svnbase="http://llvm.org/svn/llvm-project"
    local svn=$1
    local dir=$2
    if [ -d $dir/.svn ]; then
        (cd $dir && svn update -r $pkgver) || warning "Update failed!"
    else
        svn co $svnbase/$svn/trunk $dir --config-dir ./ -r $pkgver
    fi
}

_make_w_python2() {
    ln -sf $(which python2) python
    PATH="./:$PATH" make $@
    rm python
}

build() {
  cd "$srcdir"

  msg2 "Connecting to LLVM.org SVN server...."

  _checkout "llvm"              $_svnmod
  _checkout "cfe"               "llvm/tools/clang"
  _checkout "clang-tools-extra" "llvm/tools/clang/tools/extra"
  _checkout "compiler-rt"       "llvm/projects/compiler-rt"

  msg2 "SVN checkout done or server timeout"
  msg2 "Starting build..."

  cd $_svnmod
  ./configure --prefix=/usr CC=gcc CXX=g++
  _make_w_python2
}

package() {
  cd "$srcdir/$_svnmod"
  _make_w_python2 DESTDIR=$pkgdir install
  install -Dm644 LICENSE.TXT "$pkgdir/usr/share/licenses/$pkgname/LICENSE"
}

# vim:set ts=2 sw=2 et:
