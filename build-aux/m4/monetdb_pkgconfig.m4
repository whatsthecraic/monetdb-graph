# pkg-config wrapper for monetdb5.pc
#serial 1

# Avoid expanding the macros when the file has been included multiple times
m4_ifndef([_MONETDB_PKGCONFIG_DEFINED], [
m4_define([_MONETDB_PKGCONFIG_DEFINED])

# FIXME: remove this hack
m4_include(DIR_BUNDLE_M4[/pkg.m4])


# _MONETDB_PKGCONFIG_VARIABLE(pkg_config_variable, output_variable, [description])
# -------------------------------------------
#
# Based on PKG_CHECK_VAR from pkg.m4, store in <output_variable> the value of
# the variable <pkg_config_variable> from the package config related
# associated to monetdb5.
# The variable <output_variable> IS NOT AC_SUBSTed.
# The parameter <description> is the text printed to the console
# Differently from PKG_CHECK_VAR, the variable is not promoted as precious.
AC_DEFUN([_MONETDB_PKGCONFIG_VARIABLE],
[AC_REQUIRE([PKG_PROG_PKG_CONFIG])
AS_IF([[test x"$3" = x]],
  AC_MSG_CHECKING([for MonetDB $1]),
  AC_MSG_CHECKING([$3])
)
AS_VAR_PUSHDEF([TARGET], [$2])
_PKG_CONFIG(TARGET, [variable="][$1]["], [[monetdb5]])
AS_VAR_COPY([TARGET], [pkg_cv_]TARGET)
AS_VAR_IF([TARGET], [""],
  [AC_MSG_ERROR([unable to find the variable --$1 in the monetdb5 package config settings])],
  [AC_MSG_RESULT(["${]TARGET[}"])])
AS_VAR_POPDEF([TARGET])
])dnl _MONETDB_PKGCONFIG_VARIABLE

# MONETDB_PKGCONFIG_VARIABLE(pkg_config_variable, output_variable, [description])
# -------------------------------------------
#
# Stores in <output_variable> the value from the variable <pkg_config_variable>.
# The text <description> is printed into the stdout.
AC_DEFUN([MONETDB_PKGCONFIG_VARIABLE],[
  _MONETDB_PKGCONFIG_VARIABLE($@)
  AC_SUBST([$2])
])dnl MONETDB_PKGCONFIG_VARIABLE

# _MONETDB_PKGCONFIG_OPTION(option, output_variable)
# -------------------------------------------
#
# Stores in <output_variable> the value from the option --<pkg_config_variable>.
# The variable <output_variable> is not AC_SUBSTed.
# It does not print any message to the user regarding the check being done.
AC_DEFUN([_MONETDB_PKGCONFIG_OPTION], [dnl
  AC_REQUIRE([PKG_PROG_PKG_CONFIG])
  AS_VAR_PUSHDEF([TARGET], [$2])
  _PKG_CONFIG(TARGET, [$1], [[monetdb5]])
  AS_VAR_COPY([TARGET], [pkg_cv_]TARGET)
  AS_VAR_IF([TARGET], [""],
    [AC_MSG_ERROR([unable to find the option --$1 in the monetdb5 package config settings])])
])dnl _MONETDB_PKGCONFIG_OPTION

# MONETDB_PKGCONFIG_OPTION(option, output_variable)
# -------------------------------------------
#
# Stores in <output_variable> the value from the option --<pkg_config_variable>.
AC_DEFUN([MONETDB_PKGCONFIG_OPTION], [dnl
  _MONETDB_PKGCONFIG_OPTION($@)
  AC_SUBST([$2])
])dnl MONETDB_PKGCONFIG_OPTION


]) #ifndef _MONETDB_PKGCONFIG_DEFINED
