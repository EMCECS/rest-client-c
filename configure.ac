AC_INIT([rest-client-c], [1.0], [jason_d0t_cwik@emc_d0t_com])
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
LT_INIT
AC_PROG_CC
CFLAGS="$CFLAGS -Wall -Werror"
AC_CHECK_LIB([curl], [curl_easy_init], [CURLLIB=-lcurl])
AC_SUBST([CURLLIB])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile lib/Makefile tests/Makefile])
AC_OUTPUT
