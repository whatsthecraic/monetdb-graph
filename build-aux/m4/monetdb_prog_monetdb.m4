# Resolve the dependency on MonetDB, using the paths provided by pkg-config.
# Dependencies: monetdb_arg_enable.m4, monetdb_pkgconfig.m4
# serial 1

# MONETDB_PROG_MONETDB
# -------------------------------------------
#
# It checks whether monetdb5 can be reached through pkg-config or it is on the $PATH and
# determines the install paths for the library. It also provides the precious variable
# MONETDB to affect the search path.
# On success, it sets the following output variables:
#   MONETDB_PREFIX: the install prefix of MonetDB
#   MONETDB_INCLUDEDIR: the directory where headers have been installed
#   MONETDB_LIBDIR: the directory where the libraries have been installed
#   MONETDB_INCLUDES: the include directory to augment CFLAGS
#   MONETDB_LIBS: the link options to augment LDFLAGS
# On failure, it aborts with an error.
AC_DEFUN([MONETDB_PROG_MONETDB], [
AC_ARG_VAR([MONETDB], [The base path where MonetDB has been installed])
ac_monetdb_pkgconfig_name="monetdb5"
if (test -n "${MONETDB}"); then
    # is this the binary executable for monetdb?
    if test -f "${MONETDB}"; then
        if ( test -x "${MONETDB}" && ${MONETDB} --version | head -n1 | egrep -i 'MonetDB[[:space:]]*[[:digit:]]+[[:space:]]*server' >/dev/null 2>&1); then
            ac_monetdb_bindir=`AS_DIRNAME([["${MONETDB}"]])`
            ac_monetdb_prefix=`AS_DIRNAME([["${ac_monetdb_bindir}"]])`
            ac_monetdb_pkgconfig_dir="${ac_monetdb_prefix}:${ac_monetdb_prefix}/lib:${ac_monetdb_prefix}/lib/pkgconfig"
        else
             AC_MSG_ERROR([[invalid path specified for the variable \${MONETDB}: `${MONETDB}'. The path is a regular file and it is not pointing to MonetDB server]])
        fi
    elif ( test "x"$(basename "${MONETDB}") = "x${ac_monetdb_pkgconfig_name}.pc" ); then
        ac_monetdb_pkgconfig_dir=`AS_DIRNAME([["${MONETDB}"]])`
    elif test -d "${MONETDB}"; then # this is a directory
        # is this the bindir ?
        ac_monetdb_basename=$(basename "${MONETDB}")
        if ( test -f "${MONETDB}/mserver5" && test -x "${MONETDB}/mserver5" ); then
            ac_monetdb_bindir="${MONETDB}"
            ac_monetdb_prefix=`AS_DIRNAME([["${ac_monetdb_bindir}"]])`
            ac_monetdb_pkgconfig_dir="${ac_monetdb_prefix}:${ac_monetdb_prefix}/lib:${ac_monetdb_prefix}/lib/pkgconfig"
        # is this the prefix directory ?
        elif ( test -f "${MONETDB}/lib/pkgconfig/${ac_monetdb_pkgconfig_name}.pc" ); then
            ac_monetdb_pkgconfig_dir="${MONETDB}/lib/pkgconfig";
        # is this the libdir directory ?
        elif ( test -f "${MONETDB}/pkgconfig/${ac_monetdb_pkgconfig_name}.pc" ); then
            ac_monetdb_pkgconfig_dir="${MONETDB}/pkgconfig";
        # is this the pkgconfig directory ?
        elif ( test -f "${MONETDB}/${ac_monetdb_pkgconfig_name}.pc" ); then
            ac_monetdb_pkgconfig_dir="${MONETDB}";
        else
            AC_MSG_ERROR([[invalid path specified for the variable \${MONETDB}. Unable to find `monetdb5.pc': `${MONETDB}']])
        fi
    else
        AC_MSG_ERROR([[invalid path specified for the variable \${MONETDB}. The path does not exist: `${MONETDB}']])
    fi
fi
# Okay, the user provided nothing... Scan the system to find the library
if (test -z "$ac_monetdb_pkgconfig_dir"); then
    if pkg-config --exists monetdb5; then
        ac_monetdb_pkgconfig_found="yes"
    else    
        AC_PATH_PROG([MONETDB_MSERVER5], [mserver5])
        if (test -z "${ac_cv_path_MONETDB_MSERVER5}"); then
            AC_MSG_ERROR([unable to detect where monetdb has been installed. Use ./configure MONETDB=\${monetdb_prefix_path} to set the actual path])
        fi
        ac_monetdb_bindir=`AS_DIRNAME([["${ac_cv_path_MONETDB_MSERVER5}"]])`
        ac_monetdb_prefix=`AS_DIRNAME([["${ac_monetdb_bindir}"]])`
        ac_monetdb_pkgconfig_dir="${ac_monetdb_prefix}:${ac_monetdb_prefix}/lib:${ac_monetdb_prefix}/lib/pkgconfig"
    fi
fi
# Try again to search for the installed package, this time by augmenting the special variable PKG_CONFIG_PATH
ac_monetdb_pkgconfig_old=""
if (test -z "${ac_monetdb_pkgconfig_found}"); then
    if (test -n "${PKG_CONFIG_PATH}"); then
        ac_monetdb_pkgconfig_old="${PKG_CONFIG_PATH}"
        export PKG_CONFIG_PATH="${ac_monetdb_pkgconfig_dir}:${PKG_CONFIG_PATH}"
    else
        export PKG_CONFIG_PATH="${ac_monetdb_pkgconfig_dir}"
    fi
    if ! pkg-config --exists monetdb5; then
        AC_MSG_ERROR([[unable to detect where MonetDB has been installed. PKG_CONFIG_PATH set to: `${ac_monetdb_pkgconfig_dir}'. Use ./configure MONETDB=\${monetdb_prefix_path} to set the actual path]])
    fi
fi
# Retrieve the configuration of MonetDB from pkg-config
PKG_PROG_PKG_CONFIG
MONETDB_PKGCONFIG_VARIABLE([prefix], [MONETDB_PREFIX])
MONETDB_PKGCONFIG_VARIABLE([includedir], [MONETDB_INCLUDEDIR], [[for MonetDB headers]])
MONETDB_PKGCONFIG_VARIABLE([libdir], [MONETDB_LIBDIR], [[for MonetDB libraries]])
MONETDB_PKGCONFIG_OPTION([cflags-only-I], [MONETDB_INCLUDES])
MONETDB_PKGCONFIG_OPTION([libs], [MONETDB_LIBS])

# Reset PKG_CONFIG_PATH
export PKG_CONFIG_PATH="${ac_monetdb_pkgconfig_old}"
unset ac_monetdb_pkgconfig_old

unset ac_monetdb_pkgconfig_name
])
