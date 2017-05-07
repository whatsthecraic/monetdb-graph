# Custom macros based on pkg.m4

# 
# MONETDB_PKGCFG_VAR(CONFIG-VARIABLE, [PRINT_MESSAGE])
# -------------------------------------------
#
# Based on PKG_CHECK_VAR from pkg.m4, store in MONETDB_<config_variable>
# the value of the variable <config_variable> from the package config related 
# associated to monetdb5.
# Differently from PKG_CHECK_VAR, the variable is not promoted as precious.
AC_DEFUN([MONETDB_PKGCFG_VAR],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])dnl
AS_IF([test x"$2" = x],
  AC_MSG_CHECKING([for MonetDB $1]),
  AC_MSG_CHECKING([$2])
)
AS_VAR_PUSHDEF([TARGET], [MONETDB_]m4_toupper([$1]))
_PKG_CONFIG(TARGET, [variable="][$1]["], [[monetdb5]])
AS_VAR_COPY([TARGET], [pkg_cv_]TARGET)
AS_VAR_IF([TARGET], [""],
  [AC_MSG_ERROR([unable to find the variable --$1 in the monetdb5 package config settings])],
  [AC_MSG_RESULT(["${]TARGET[}"])])
AS_VAR_POPDEF([TARGET])
])dnl MONETDB_PKGCFG_VAR