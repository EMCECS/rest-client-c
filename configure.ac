AC_INIT([rest-client-c], [1.0.1], [jason_d0t_cwik@emc_d0t_com])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
LT_INIT
AC_PROG_CC
AX_CFLAGS_WARN_ALL
AX_CHECK_COMPILE_FLAG([-Werror], [CFLAGS="$CFLAGS -Werror"], [], [])
#AC_CHECK_LIB([curl], [curl_easy_init], [CURLLIB=-lcurl])
#AC_SUBST([CURLLIB])
PKG_CHECK_MODULES(CURL, libcurl >= 7.20)
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile lib/Makefile tests/Makefile lib/rest-client-c.pc])
AC_OUTPUT
