require gcc-configure-common.inc

LICENSE = "NCSA | MIT"

LIC_FILES_CHKSUM = "\
    file://libsanitizer/LICENSE.TXT;md5=0249c37748936faf5b1efd5789587909 \
"

EXTRA_OECONF_PATHS = "\
    --with-sysroot=/not/exist \
    --with-build-sysroot=${STAGING_DIR_TARGET} \
"

# Workaround for this issue: https://github.com/llvm/llvm-project/issues/65144 on aarch64
# compiler-rt/sanitizers/aarch64: CHECK failed: sanitizer_allocator_primary64.h:133
CXXFLAGS:append:aarch64 = " -DSANITIZER_CAN_USE_ALLOCATOR64=0"

do_configure () {
    rm -rf ${B}/${TARGET_SYS}/libsanitizer/
    mkdir -p ${B}/${TARGET_SYS}/libsanitizer/
    cd ${B}/${TARGET_SYS}/libsanitizer/
    chmod a+x ${S}/libsanitizer/configure
    relpath=${@os.path.relpath("${S}/libsanitizer", "${B}/${TARGET_SYS}/libsanitizer")}
    $relpath/configure ${CONFIGUREOPTS} ${EXTRA_OECONF}
    # Easiest way to stop bad RPATHs getting into the library since we have a
    # broken libtool here
    sed -i -e 's/hardcode_into_libs=yes/hardcode_into_libs=no/' ${B}/${TARGET_SYS}/libsanitizer/libtool
    # Link to the sysroot's libstdc++ instead of one gcc thinks it just built
    sed -i -e '/LIBSTDCXX_RAW_CXX_\(CXXFLAGS\|LDFLAGS\)\s*=/d' ${B}/${TARGET_SYS}/libsanitizer/*/Makefile
}
EXTRACONFFUNCS += "extract_stashed_builddir"
do_configure[depends] += "${COMPILERDEP}"

do_compile () {
    cd ${B}/${TARGET_SYS}/libsanitizer/
    oe_runmake MULTIBUILDTOP=${B}/${TARGET_SYS}/libsanitizer/
}

do_install () {
    cd ${B}/${TARGET_SYS}/libsanitizer/
    oe_runmake 'DESTDIR=${D}' MULTIBUILDTOP=${B}/${TARGET_SYS}/libsanitizer/ install
    if [ -d ${D}${libdir}/gcc/${TARGET_SYS}/${BINV}/include ]; then
	install -d ${D}${libdir}/${TARGET_SYS}/${BINV}/include
	mv ${D}${libdir}/gcc/${TARGET_SYS}/${BINV}/include/* ${D}${libdir}/${TARGET_SYS}/${BINV}/include
	rmdir --ignore-fail-on-non-empty -p ${D}${libdir}/gcc/${TARGET_SYS}/${BINV}/include
    fi
    if [ -d ${D}${infodir} ]; then
        rmdir --ignore-fail-on-non-empty -p ${D}${infodir}
    fi
    chown -R root:root ${D}
}

INHIBIT_DEFAULT_DEPS = "1"
ALLOW_EMPTY:${PN} = "1"
DEPENDS = "virtual/crypt gcc-runtime virtual/cross-cc"

# used to fix ../../../../../../../../../work-shared/gcc-8.3.0-r0/gcc-8.3.0/libsanitizer/libbacktrace/../../libbacktrace/elf.c:772:21: error: 'st.st_mode' may be used uninitialized in this function [-Werror=maybe-uninitialized]
DEBUG_OPTIMIZATION:append = " -Wno-error"

BBCLASSEXTEND = "nativesdk"

PACKAGES = "${PN} ${PN}-dbg"
PACKAGES += "libasan libubsan liblsan libtsan"
PACKAGES += "libasan-dev libubsan-dev liblsan-dev libtsan-dev"
PACKAGES += "libasan-staticdev libubsan-staticdev liblsan-staticdev libtsan-staticdev"

RDEPENDS:libasan += "libstdc++"
RDEPENDS:libubsan += "libstdc++"
RDEPENDS:liblsan += "libstdc++"
RDEPENDS:libtsan += "libstdc++"
RDEPENDS:libasan-dev += "${PN}"
RDEPENDS:libubsan-dev += "${PN}"
RDEPENDS:liblsan-dev += "${PN}"
RDEPENDS:libtsan-dev += "${PN}"
RRECOMMENDS:${PN} += "libasan libubsan"
RRECOMMENDS:${PN}:append:x86 = " liblsan"
RRECOMMENDS:${PN}:append:x86-64 = " liblsan libtsan"
RRECOMMENDS:${PN}:append:powerpc64 = " liblsan libtsan"
RRECOMMENDS:${PN}:append:aarch64 = " liblsan libtsan"
RRECOMMENDS:${PN}:append:riscv64 = " liblsan libtsan"

do_package_write_ipk[depends] += "virtual/${MLPREFIX}compilerlibs:do_packagedata"
do_package_write_deb[depends] += "virtual/${MLPREFIX}compilerlibs:do_packagedata"
do_package_write_rpm[depends] += "virtual/${MLPREFIX}compilerlibs:do_packagedata"

# Only x86, powerpc, sparc, s390, arm, aarch64 and loongarch64 are supported
COMPATIBLE_HOST = '(x86_64|i.86|powerpc|sparc|s390|arm|aarch64|loongarch64|riscv64).*-linux'
# musl is currently broken entirely
COMPATIBLE_HOST:libc-musl = 'null'

FILES:libasan += "${libdir}/libasan.so.* ${libdir}/libhwasan.so.*"
FILES:libasan-dev += "\
    ${libdir}/libasan_preinit.o \
    ${libdir}/libhwasan_preinit.o \
    ${libdir}/libasan.so \
    ${libdir}/libhwasan.so \
    ${libdir}/libasan.la \
"
FILES:libasan-staticdev += "${libdir}/libasan.a \
    ${libdir}/libhwasan.a \
"

FILES:libubsan += "${libdir}/libubsan.so.*"
FILES:libubsan-dev += "\
    ${libdir}/libubsan.so \
    ${libdir}/libubsan.la \
"
FILES:libubsan-staticdev += "${libdir}/libubsan.a"

FILES:liblsan += "${libdir}/liblsan.so.*"
FILES:liblsan-dev += "\
    ${libdir}/liblsan.so \
    ${libdir}/liblsan.la \
    ${libdir}/liblsan_preinit.o \
"
FILES:liblsan-staticdev += "${libdir}/liblsan.a"

FILES:libtsan += "${libdir}/libtsan.so.*"
FILES:libtsan-dev += "\
    ${libdir}/libtsan.so \
    ${libdir}/libtsan.la \
    ${libdir}/libtsan_*.o \
"
FILES:libtsan-staticdev += "${libdir}/libtsan.a"

FILES:${PN} = "${libdir}/*.spec ${libdir}/${TARGET_SYS}/${BINV}/include/sanitizer/*.h"
