AC_INIT([rest-client-c], [1.0.2], [jason_d0t_cwik@emc_d0t_com])
AC_CONFIG_AUX_DIR([build-aux])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
LT_INIT
AC_PROG_CC
AX_CFLAGS_WARN_ALL
AX_CHECK_COMPILE_FLAG([-Werror], [CFLAGS="$CFLAGS -Werror"], [], [])
#AC_CHECK_LIB([curl], [curl_easy_init], [CURLLIB=-lcurl])
#AC_SUBST([CURLLIB])
PKG_CHECK_MODULES(CURL, libcurl >= 7.20)
AC_ARG_ENABLE(threads, AC_HELP_STRING([--enable-threads], 
		[enable pthreads (default is yes)]), 
		ac_enable_threads=$enableval, 
		ac_enable_threads=yes) 
if test "$ac_enable_threads" = yes; then 
	AC_DEFINE(_PTHREADS, 1, [use threads]) 
	AX_PTHREAD
fi
AM_CONDITIONAL(THREADS, test $ac_enable_threads = yes) 
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile lib/Makefile tests/Makefile lib/rest-client-c.pc])
AC_OUTPUT
