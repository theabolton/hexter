dnl Process this file with autoconf to produce a configure script.

AC_INIT(hexter, 0.6.0, musound-at-jps-dot-net)

AC_CONFIG_SRCDIR(src/hexter.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_HEADER_STDC

AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_PROG_LIBTOOL

AC_C_BIGENDIAN

AC_ARG_WITH(textui, AC_HELP_STRING([--with-textui], [build text-only user interface (disables GTK+ GUI)]),
            [ if test $withval = "yes"; then with_textui=yes ;
              else with_textui=no ; fi ], with_textui=no)
AC_ARG_WITH(gtk2, AC_HELP_STRING([--with-gtk2], [use GTK+ 2.0 default=yes]),
            [ if test $withval = "yes"; then try_gtk2=yes ;
              else try_gtk2=no ; fi ], try_gtk2=yes)
with_gtk=no
if test "x${with_textui}" = 'xno'; then
  dnl Check for GTK+
  dnl First look for GTK+ 2.x, unless '--without-gtk2' was specified.
  if test "x${try_gtk2}" = 'xyes'; then
      AM_PATH_GTK_2_0(2.0.0, with_gtk='yes (version 2.0)', with_gtk=no)
  fi
  dnl  No GTK+ 2.x found, look for GTK+ 1.2 instead
  if test "x${with_gtk}" = 'xno'; then
      AM_PATH_GTK(1.2.0, with_gtk='yes (version 1.2)', AC_MSG_ERROR([error: no version of GTK+ found (maybe you want '--with-textui'?)]))
  fi
else
  dnl Check for readline and (n)curses/termcap
  AC_CHECK_HEADER([readline/readline.h], , AC_MSG_ERROR([error: no readline.h found]))
  AC_CHECK_HEADER([readline/history.h], , AC_MSG_ERROR([error: no history.h found]))
  READLINE_LIBS=''
  AC_CHECK_LIB(readline, readline, READLINE_LIBS='-lreadline -lcurses',, [-lcurses])
  if test "x$READLINE_LIBS" = 'x'; then
    dnl cache doesn't consider OTHER-LIBRARIES:
    unset ac_cv_lib_readline_readline
    AC_CHECK_LIB(readline, readline, READLINE_LIBS='-lreadline -ltermcap',, [-ltermcap])
  fi
  if test "x$READLINE_LIBS" = 'x'; then
    AC_MSG_ERROR([error: could not link readline libraries])
  fi
  AC_SUBST(READLINE_LIBS)
  AC_DEFINE(BUILD_TEXTUI, 1, Define to 1 if building the text-only user interface)
  with_gtk='no (building text-only user interface)'
fi
AM_CONDITIONAL(BUILD_TEXTUI, test "x${with_textui}" = 'xyes')
echo "GTK support: $with_gtk"

dnl OS specific checks
case "${host_os}" in
darwin*)
  darwin=yes
  PKG_CHECK_MODULES(ALSA, libdssialsacompat)
  #AC_DEFINE(DARWIN, 1, [Define if building for Mac OS X Darwin])
  AC_DEFINE(MIDI_COREMIDI, 1, [Define for CoreMIDI MIDI support])
  ;;
*)
  darwin=no
  PKG_CHECK_MODULES(ALSA, alsa)
  AC_DEFINE(MIDI_ALSA, 1, [Define for ALSA MIDI support])
  ;;
esac
AM_CONDITIONAL(DARWIN, test x$darwin = xyes)

dnl Require DSSI and liblo
PKG_CHECK_MODULES(MODULE, dssi >= 0.4 liblo >= 0.12)

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
CFLAGS="$CFLAGS $ALSA_CFLAGS $MODULE_CFLAGS"
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
  *) CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math -Winline" ;;
  esac
  # add '-finline-limit=...' only for GCC 3.x:
  case `$CC -dumpversion 2>/dev/null` in
  3.*) CFLAGS="$CFLAGS -finline-limit=5000" ;;
  esac
fi
changequote([,])dnl

AC_OUTPUT([
Makefile
src/Makefile
])

