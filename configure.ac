dnl Process this file with autoconf to produce a configure script.

AC_INIT(hexter, 1.0.1, sean-at-smbolton-dot-com)

AC_CONFIG_SRCDIR(src/hexter.c)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC

AC_ENABLE_STATIC(no)
AC_ENABLE_SHARED(yes)
AC_PROG_LIBTOOL

AC_C_BIGENDIAN

AC_ARG_WITH(textui, AC_HELP_STRING([--with-textui],
                                        [build text-only user interface]),
            [ if test $withval = "yes"; then with_textui=yes ;
              else with_textui=no ; fi ], with_textui=no)
AC_ARG_WITH(gtk2, AC_HELP_STRING([--with-gtk2],
                                        [build GTK+ 2.0 user interface]),
            [ if test $withval = "yes"; then try_gtk2=yes ;
              else try_gtk2=no ; fi ], try_gtk2=yes)

dnl Check for GTK
if test "x${try_gtk2}" = 'xyes'; then
  AM_PATH_GTK_2_0(2.4.0, with_gtk2=yes, with_gtk2=no)
fi
if test "x${with_gtk2}" = 'xyes'; then
  AC_DEFINE(BUILD_GTKUI, 1, Define to 1 if building the GTK user interface)
fi

dnl Text UI
if test "x${with_textui}" = 'xyes'; then
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
fi
AM_CONDITIONAL(BUILD_TEXTUI, test "x${with_textui}" = 'xyes')
AM_CONDITIONAL(BUILD_GTKUI, test "x${with_gtk2}" = 'xyes')

dnl floating point
AC_ARG_ENABLE(floating-point,
              AC_HELP_STRING([--enable-floating-point],
                             [enable floating point rendering, instead of fixed point, default=no]),
              [ if test $enableval = "yes"; then
                  AC_DEFINE(HEXTER_USE_FLOATING_POINT, 1, [Define to 1 to enable floating-point rendering.])
                fi ],
              enable_floating_point=no)

dnl Check for LADSPA
AC_CHECK_HEADERS(ladspa.h)

dnl Require DSSI and liblo
PKG_CHECK_MODULES(MODULE, dssi >= 0.4 liblo >= 0.12)

dnl OS specific checks
case "${host_os}" in
darwin*)
  darwin=yes
  PKG_CHECK_MODULES(ALSA, libdssialsacompat)
  AC_DEFINE(MIDI_COREMIDI, 1, [Define for CoreMIDI MIDI support])
  ;;
freebsd*)
  darwin=no
  PKG_CHECK_MODULES(ALSA, libdssialsacompat)
  ;;
*)
  darwin=no
  PKG_CHECK_MODULES(ALSA, alsa)
  AC_DEFINE(MIDI_ALSA, 1, [Define for ALSA MIDI support])
  ;;
esac
AM_CONDITIONAL(DARWIN, test x$darwin = xyes)

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
  *)  CFLAGS="$CFLAGS -fomit-frame-pointer -funroll-loops -finline-functions -ffast-math -Winline"
      # add '-finline-limit=...' only for GCC 3.x and 4.x:
      case `$CC -dumpversion 2>/dev/null` in
      # these --params are also in 3.4.6, but not in 3.3.6
      4.*) CFLAGS="$CFLAGS -finline-limit=5000 --param large-function-growth=4900 --param inline-unit-growth=900" ;;
      3.*) CFLAGS="$CFLAGS -finline-limit=5000" ;;
      esac ;;
  esac
fi
changequote([,])dnl

AC_OUTPUT([
Makefile
src/Makefile
])

echo "====== hexter ${PACKAGE_VERSION} configured ======"
echo "Floating point render enabled:      $enable_floating_point"
echo "Building GTK 2.0 user interface:    $with_gtk2"
echo "Building text-only user interface:  $with_textui"

