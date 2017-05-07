#############################################################################
# Initialization
AC_PREREQ([2.69])
AC_INIT([monetdb-graph], [0.0.1])
AC_CONFIG_AUX_DIR([build-aux])
AC_PRESERVE_HELP_ORDER
AC_LANG([C++])

#############################################################################
# Template makefiles
# Bulk of output variables replaced by autoconf
AC_CONFIG_FILES([common.mk])
# Main makefile
AC_CONFIG_FILES([Makefile])

#############################################################################
# Set the compiler $CC and $CXX. Prefer clang over gcc
CFLAGS=${CFLAGS:+""}; CXXFLAGS=${CXXFLAGS:+""}; dnl Do not automatically set -g -O2
AC_PROG_CC([clang gcc icc cc])
AC_PROG_CXX([clang++ g++ icpc c++ cxx])
AX_CXX_COMPILE_STDCXX_17 dnl Ask for C++17, thanks!

#############################################################################
# Check for the `install' program and provide the INSTALL output variables
AC_PROG_INSTALL

#############################################################################
# Check whether the user has explicitly set CPPFLAGS, CFLAGS and CXXFLAGS. If 
# so we try to avoid polluting these flags and respect the user setting
m4_divert_push([INIT_PREPARE]) 
if test "x${CPPFLAGS}" != "x"; then ac_user_cppflags="yes"; fi
if test "x${CFLAGS}" != "x"; then ac_user_cflags="yes"; fi
if test "x${CXXFLAGS}" != "x"; then ac_user_cxxflags="yes"; fi
m4_divert_pop([INIT_PREPARE])

#############################################################################
# As we are going to build a shared library, enable PIC
AS_VAR_APPEND([[EXTRA_CFLAGS]], [[" -fPIC"]])
AS_VAR_APPEND([[EXTRA_CXXFLAGS]], [[" -fPIC"]])

#############################################################################
# MonetDB installation path. Set the variables MONETDB_PREFIX
# MONETDB_INCLUDEDIR, MONETDB_LIBDIR and MONETDB_INCLUDES
# First check whether the user was so polite to provide the actual path
AC_ARG_VAR([MONETDB], [The base path where MonetDB has been installed])
ac_pkg_config_name="monetdb5"
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
    elif ( test "x$(basename("${MONETDB}"))" = "x${ac_pkg_config_name}.pc" ); then
        ac_monetdb_pkgconfig_dir=`AS_DIRNAME([["${MONETDB}"]])`
    elif test -d "${MONETDB}"; then # this is a directory
        # is this the bindir ?
        ac_monetdb_basename=$(basename("${MONETDB}"))
        if ( test -f "${MONETDB}/mserver5" && test -x "${MONETDB}/mserver5" ); then
            ac_monetdb_bindir="${MONETDB}"
            ac_monetdb_prefix=`AS_DIRNAME([["${ac_monetdb_bindir}"]])`
            ac_monetdb_pkgconfig_dir="${ac_monetdb_prefix}:${ac_monetdb_prefix}/lib:${ac_monetdb_prefix}/lib/pkgconfig"
        # is this the prefix directory ?
        elif ( test -f "${MONETDB}/lib/pkgconfig/${ac_pkg_config_name}.pc" ); then
            ac_monetdb_pkgconfig_dir="${MONETDB}/lib/pkgconfig";
        # is this the libdir directory ?
        elif ( test -f "${MONETDB}/pkgconfig/${ac_pkg_config_name}.pc" ); then
            ac_monetdb_pkgconfig_dir="${MONETDB}/pkgconfig";
        # is this the pkgconfig directory ?
        elif ( test -f "${MONETDB}/${ac_pkg_config_name}.pc" ); then
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
MONETDB_PKGCFG_VAR([prefix]) dnl yields MONETDB_PREFIX
MONETDB_PKGCFG_VAR([includedir], [[for MonetDB headers]]) dnl yields MONETDB_INCLUDEDIR
MONETDB_PKGCFG_VAR([libdir], [[for MonetDB libraries]]) dnl yiedls MONETDB_LIBDIR
MONETDB_INCLUDES=$(pkg-config --cflags-only-I "${ac_pkg_config_name}")
MONETDB_LIBS=$(pkg-config --libs "${ac_pkg_config_name}")

# Reset PKG_CONFIG_PATH
export PKG_CONFIG_PATH="${ac_monetdb_pkgconfig_old}"
unset ac_monetdb_pkgconfig_old

#############################################################################
# MonetDB output variables
AC_SUBST([MONETDB_PREFIX])
AC_SUBST([MONETDB_INCLUDEDIR])
AC_SUBST([MONETDB_LIBDIR])
AC_SUBST([MONETDB_INCLUDES])
AC_SUBST([MONETDB_LIBS])

#############################################################################
# Enable asserts? Possible values:
# yes => nop
# no => EXTRA_CPPFLAGS += -DNDEBUG
# auto => check whether MonetDB defines NDEBUG and set EXTRA_CPPFLAGS accordingly
MONETDB_ARG_ENABLE([assert], 
    [Whether to enable assertions. The option 'auto' implies the actual value is inherited from MonetDB config (recommended)], 
    [yes no auto], [auto]) 
    
if (test x"${enable_assert}" = x"auto"); then
    ac_cppflags_old="${CPPFLAGS}"

    AC_LANG_PUSH([C])
    CPPFLAGS="${CPPFLAGS} ${MONETDB_INCLUDES}"
    AC_MSG_CHECKING([whether to enable assertions])
    # call AC_LANG_CONFTEST() directly as AC_LANG_CONFTEST(C) is going to prepend
    # all the package definitions from AC_INIT, causing conflicts with monetdb_config.h
    m4_indir([AC_LANG_CONFTEST()], [[
        #include "monetdb_config.h"
        int main(){
        #if defined(NDEBUG) && NDEBUG != 0
            #error Asserts disabled
        #endif
            return 0;
        }
    ]])
    # when the first param is empty, it refers to conftest.$ac_ext
    AC_COMPILE_IFELSE([], [enable_assert=yes], [enable_assert=no])
    AC_MSG_RESULT(${enable_assert})

    CPPFLAGS="${ac_cppflags_old}"
    AC_LANG_POP([C])
fi
if (test x"${enable_assert}" = x"yes"); then
    : ; # nop
elif (test x"${enable_assert}" = x"no"); then
    EXTRA_CPPFLAGS="${EXTRA_CPPFLAGS} -DNDEBUG"
else
    AC_MSG_ERROR([Invalid value for --enable-assert: ${enable_assert}])
fi

#############################################################################
# Debug flags (-g)
MONETDB_ARG_ENABLE([debug], 
    [Whether to enable the debug flags], 
    [yes no], [yes]) 
    
dnl _monetdb_set_debug_flags([CFLAGS], [C])
dnl _monetdb_set_debug_flags([CXXFLAGS], [C++])
dnl first argument is the variable with the flags, the second argument is the language
m4_defun([_monetdb_set_debug_flags], [
    m4_pushdef([_FLAGS], [m4_translit([$1], [+], [X])FLAGS]) dnl => CFLAGS, CXXFLAGS
    [if test -n "${ac_user_]m4_tolower(_FLAGS)[}"; then]
         AC_MSG_WARN([Action --enable-debug ignored as _FLAGS has been explicitly set through command line])
    else
        AS_VAR_APPEND([_FLAGS], [[" -g"]])
        # Force clang to emit the whole debug information
        AC_LANG_PUSH([$1])
        MONETDB_SET_CC_FLAG([_FLAGS], [-fno-limit-debug-info])
        AC_LANG_POP([$1])
        
    fi
    m4_popdef([_FLAGS])
])
if( test x"${enable_debug}" = x"yes" ); then
    _monetdb_set_debug_flags([C])
    _monetdb_set_debug_flags([C++])
fi
m4_undefine([_monetdb_set_debug_flags])

#############################################################################
# Warning flags (-Wall)
MONETDB_ARG_ENABLE([warnings], 
    [Whether to enable all warnings (-Wall)], 
    [yes no], [yes])
m4_defun([_monetdb_set_warnings], [
    m4_pushdef([_FLAGS], [m4_translit([$1], [+], [X])FLAGS]) dnl => CFLAGS, CXXFLAGS
    [if test -n "${ac_user_]m4_tolower(_FLAGS)[}"; then]
         AC_MSG_WARN([Action --enable-warnings ignored as _FLAGS has been explicitly set through command line])
    [else]
        AS_VAR_APPEND([_FLAGS], [" -Wall"])
    [fi]
    m4_popdef([_FLAGS])
])
if( test x"${enable_warnings}" = x"yes" ); then
    _monetdb_set_warnings([C])
    _monetdb_set_warnings([CXX])
fi

#############################################################################
# Optimization flags (-O3)
MONETDB_ARG_ENABLE([optimize], [Whether to enable the optimization flags], [yes no], [no])

m4_defun([_monetdb_set_optimization_flags], [
    m4_pushdef([_FLAGS], [m4_translit([$1], [+], [X])FLAGS]) dnl => CFLAGS, CXXFLAGS
    [if test -n "${ac_user_]m4_tolower(_FLAGS)[}"; then]
        AC_MSG_WARN([Action --enable-optimize ignored as _FLAGS has been explicitly set through command line])
    [else]
        if( test x"${enable_optimize}" = x"yes" ); then
            AS_VAR_APPEND([_FLAGS], [[" -O3"]])
            AC_LANG_PUSH([$1])
            MONETDB_SET_CC_FLAG([_FLAGS], [-march=native])
            MONETDB_SET_CC_FLAG([_FLAGS], [-mtune=native])
            MONETDB_SET_CC_FLAG([_FLAGS], [-fno-stack-protector])
            AC_LANG_POP([$1])
        else
            AS_VAR_APPEND([_FLAGS], [[" -O0"]])
        fi
    [fi]
    m4_popdef([_FLAGS])
])
_monetdb_set_optimization_flags([C])
_monetdb_set_optimization_flags([C++])
m4_undefine([_monetdb_set_optimization_flags])

#############################################################################
# Remove extra blanks from our variables
EXTRA_CPPFLAGS=$(echo ${EXTRA_CPPFLAGS} | xargs)
CPPFLAGS=$(echo ${CPPFLAGS} | xargs);
CFLAGS=$(echo ${CFLAGS} | xargs);
EXTRA_CFLAGS=$(echo ${EXTRA_CFLAGS} | xargs);
CXXFLAGS=$(echo ${CXXFLAGS} | xargs);
EXTRA_CXXFLAGS=$(echo ${EXTRA_CXXFLAGS} | xargs);
# these two variables are only for presentation, overriding won't achieve much
ALL_CFLAGS=$(echo ${EXTRA_CPPFLAGS} ${CPPFLAGS} ${EXTRA_CFLAGS} ${CFLAGS} | xargs)
ALL_CXXFLAGS=$(echo ${EXTRA_CPPFLAGS} ${CPPFLAGS} ${EXTRA_CXXFLAGS} ${CXXFLAGS} | xargs)

#############################################################################
# CC, CXX and linker additional output variables
AC_SUBST([EXTRA_CPPFLAGS])
AC_SUBST([EXTRA_CFLAGS])
AC_SUBST([EXTRA_CXXFLAGS])

#############################################################################
# Create the configure script
AC_OUTPUT

#############################################################################
# Final summary
echo \
"-------------------------------------------------
${PACKAGE_NAME} version ${PACKAGE_VERSION}
Prefix..............: ${prefix}
Compiler C..........: ${CC} ${ALL_CFLAGS}
Compiler C++........: ${CXX} ${ALL_CXXFLAGS}
Enable assertions...: ${enable_assert}

Now type 'make @<:@<target>@:>@'
where the optional <target> is:
all
- build all binaries
install
- install everything
--------------------------------------------------"