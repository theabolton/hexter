dnl Process this file with autoconf to produce a configure script.

AC_INIT(hexter, 0.5.7, musound-at-jps-dot-net)

AC_CONFIG_SRCDIR(src/hexter.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_ISC_POSIX
AC_PROG_CC
AM_PROG_CC_STDC
AC_HEADER_STDC

AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_LIBTOOL_DLOPEN
dnl AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_C_BIGENDIAN

dnl Require GTK+ 1.2.x
AM_PATH_GTK(1.2.0, ,
            AC_MSG_ERROR(Cannot find GTK: Is gtk-config in path?))

dnl Require DSSI and liblo
PKG_CHECK_MODULES(MODULE, dssi >= 0.4 liblo >= 0.12)

dnl Require ALSA
PKG_CHECK_MODULES(ALSA, alsa)

dnl dnl Set PACKAGE_DATA_DIR in config.h.
dnl if test "x${datadir}" = 'x${prefix}/share'; then
dnl   if test "x${prefix}" = "xNONE"; then
dnl     AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${ac_default_prefix}/share/${PACKAGE}")
dnl   else
dnl     AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${prefix}/share/${PACKAGE}")
dnl   fi
dnl else
dnl   AC_DEFINE_UNQUOTED(PACKAGE_DATA_DIR, "${datadir}/${PACKAGE}")
dnl fi

dnl dnl Set PACKAGE_SOURCE_DIR in config.h.
dnl packagesrcdir=`cd $srcdir && pwd`
dnl AC_DEFINE_UNQUOTED(PACKAGE_SOURCE_DIR, "${packagesrcdir}")

dnl Use lotsa flags if we have gcc.
CFLAGS="$CFLAGS $MODULE_CFLAGS"
LDFLAGS="$LDFLAGS $MODULE_LIBS"
changequote(,)dnl
if test "x$GCC" = "xyes"; then
  case " $CFLAGS " in
  *[\ \	]-Wall[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -Wall" ;;
  esac
  case " $CFLAGS " in
  *[\ \	]-O[1-9][\ \	]*) ;;
  *) CFLAGS="$CFLAGS -O2" ;;
  esac
  case " $CFLAGS " in
  *[\ \	]-ffast-math[\ \	]*) ;;
  *) CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math -finline-limit=5000 -Winline" ;;
  esac
fi
changequote([,])dnl

AC_OUTPUT([
Makefile
src/Makefile
])

