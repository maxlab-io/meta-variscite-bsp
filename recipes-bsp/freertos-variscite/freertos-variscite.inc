# Copyright (C) 2021-2023 Variscite
DESCRIPTION = "Variscite i.MX Cortex M4/M7 Firmware Demos"
LICENSE = "BSD-3-Clause"
LIC_FILES_CHKSUM = "file://COPYING-BSD-3;md5=0858ec9c7a80c4a2cf16e4f825a2cc91"
SECTION = "BSP"

SRC_URI = " \
    file://variscite-rproc-linux \
    file://variscite-rproc-u-boot \
    file://variscite-rproc-common.sh \
    file://variscite-rproc.conf \
"

# inherit cmake
DEPENDS = "cmake-native"
DEPENDS:append = " cortexm-toolchain-cross (=${CM_GCC})"

S = "${WORKDIR}/git"

MCUXPRESSO_BRANCH = "mcuxpresso_sdk_${PV}-var01"

INSTALL_SCRIPTS="yes"
INSTALL_SCRIPTS:mx9-nxp-bsp="no"

CM_BOARD="INVALID"
CM_BOARD:mx8mn-nxp-bsp="som_mx8mn"
CM_BOARD:mx8mq-nxp-bsp="dart_mx8mq"
CM_BOARD:mx8mm-nxp-bsp="dart_mx8mm"
CM_BOARD:mx8mp-nxp-bsp="dart_mx8mp som_mx8mp"
CM_BOARD:mx8qm-nxp-bsp="som_mx8qm"
CM_BOARD:mx8qxp-nxp-bsp="som_mx8qx"
CM_BOARD:mx9-nxp-bsp="mcimx93evk"

CM_BUILD_TARGETS = " \
    debug \
    ddr_debug \
"
CM_BUILD_TARGETS:mx93-nxp-bsp = " \
    release \
"

# Firmware source directories
CM_DEMOS = " \
    multicore_examples/rpmsg_lite_str_echo_rtos \
    multicore_examples/rpmsg_lite_pingpong_rtos/linux_remote \
    demo_apps/hello_world \
"

# Firmware source directories
CM_DEMOS:mx8qm-nxp-bsp = " \
    multicore_examples/rpmsg_lite_str_echo_rtos/cm4_core0 \
    multicore_examples/rpmsg_lite_pingpong_rtos/linux_remote/cm4_core0 \
    demo_apps/hello_world/cm4_core0 \
    multicore_examples/rpmsg_lite_str_echo_rtos/cm4_core1 \
    multicore_examples/rpmsg_lite_pingpong_rtos/linux_remote/cm4_core1 \
    demo_apps/hello_world/cm4_core1 \
"

CM_DEMOS:mx93-nxp-bsp = " \
    demo_apps/ethosu_apps_rpmsg \
"

# If remoteproc is used to reload ddr firmware, special disable_cache firmware
# must be reloaded between stop and start comments
# For Example:
#   echo cm_rpmsg_lite_pingpong_rtos_linux_remote.elf.ddr_debug > /sys/class/remoteproc/remoteproc0/firmware
#   echo start > /sys/class/remoteproc/remoteproc0/state
#   echo stop > /sys/class/remoteproc/remoteproc0/state

#   echo cm_disable_cache.elf.debug > /sys/class/remoteproc/remoteproc0/firmware
#   echo start > /sys/class/remoteproc/remoteproc0/state
#   echo stop > /sys/class/remoteproc/remoteproc0/state

#   echo cm_rpmsg_lite_str_echo_rtos_imxcm4.elf.ddr_debug > /sys/class/remoteproc/remoteproc0/firmware
#   echo start > /sys/class/remoteproc/remoteproc0/state
#   echo stop > /sys/class/remoteproc/remoteproc0/state
CM_DEMOS_DISABLE_CACHE = ""
CM_DEMOS_DISABLE_CACHE:mx8qxp-nxp-bsp = "demo_apps/disable_cache"
CM_DEMOS_DISABLE_CACHE:mx8qm-nxp-bsp = "demo_apps/disable_cache/cm4_core0 demo_apps/disable_cache/cm4_core1"

DISABLE_CACHE_PATCH="disable_cache.patch"
DISABLE_CACHE_PATCH:imx8qxp-var-som="disable_cache_${PV}.patch"
DISABLE_CACHE_PATCH:imx8qxpb0-var-som="disable_cache_${PV}.patch"
SRC_URI:append:imx8qxp-var-som = " file://${DISABLE_CACHE_PATCH};apply=no"
SRC_URI:append:imx8qxpb0-var-som = " file://${DISABLE_CACHE_PATCH};apply=no"
SRC_URI:append:imx8qm-var-som = " file://${DISABLE_CACHE_PATCH};apply=no"

compile_fw() {
    DIR_GCC="$1"
    cd ${DIR_GCC}
    ./clean.sh
    CFLAGS="" CXXFLAGS="" ./build_all.sh > /dev/null
}

do_compile() {
    # Configure ARMGCC
    export ARMGCC_DIR="${STAGING_DIR_NATIVE}${bindir}/cortexm-toolchain-cross-${CM_GCC}"

    # Copy and patch hello_world demo to disable_cache demo
    if [ -e "${WORKDIR}/${DISABLE_CACHE_PATCH}" ]; then
        # Copy hello_world demo
        cp -r ${S}/boards/${CM_BOARD}/demo_apps/hello_world/ ${S}/boards/${CM_BOARD}/demo_apps/disable_cache
        # Rename hello_world strings to disable_cache
        grep -rl hello_world ${S}/boards/${CM_BOARD}/demo_apps/disable_cache | xargs sed -i 's/hello_world/disable_cache/g'
        # Rename hello_world files to disable_cache
        find ${S}/boards/${CM_BOARD}/demo_apps/disable_cache/ -name '*hello_world*' -exec sh -c 'mv "$1" "$(echo "$1" | sed s/hello_world/disable_cache/)"' _ {} \;
    fi

    # Build all demos in CM_DEMOS
    for CM_DEMO in ${CM_DEMOS}; do
        compile_fw "${S}/boards/${CM_BOARD}/${CM_DEMO}/armgcc"
    done

    # Build firmware to reset cache
    if [ -e "${WORKDIR}/${DISABLE_CACHE_PATCH}" ]; then
        # Apply patch to disable cache for machine
        cd ${S} && git apply ${WORKDIR}/${DISABLE_CACHE_PATCH}

        # Build the firmware
        for CM_DEMO in ${CM_DEMOS_DISABLE_CACHE}; do
                compile_fw "${S}/boards/${CM_BOARD}/${CM_DEMO}/armgcc"
        done
    fi
}

do_install() {
    if [ "${INSTALL_SCRIPTS}" = "yes" ]; then
        # Install scripts
        install -d ${D}${sysconfdir}/remoteproc
        install -m 0755 ${WORKDIR}/variscite-rproc-u-boot ${D}${sysconfdir}/remoteproc
        install -m 0755 ${WORKDIR}/variscite-rproc-linux ${D}${sysconfdir}/remoteproc
        install -m 0644 ${WORKDIR}/variscite-rproc-common.sh ${D}${sysconfdir}/remoteproc
        install -m 0644 ${WORKDIR}/variscite-rproc.conf ${D}${sysconfdir}/remoteproc
    fi

    # Install firmware
    install -d ${D}${nonarch_base_libdir}/firmware/
    install -d ${D}/boot/

    # Install all demos in CM_DEMOS
    for CM_DEMO in ${CM_DEMOS}; do
        DIR_GCC="${S}/boards/${CM_BOARD}/${CM_DEMO}/armgcc"
        # Install all build targets
        for CM_BUILD_TARGET in ${CM_BUILD_TARGETS}; do
            # Install elf
            FILE_CM_FW="$(basename ${DIR_GCC}/${CM_BUILD_TARGET}/*.elf)"
            install -m 644 ${DIR_GCC}/${CM_BUILD_TARGET}/${FILE_CM_FW} ${D}${nonarch_base_libdir}/firmware/cm_${FILE_CM_FW}.${CM_BUILD_TARGET}

            # Install bin
            FILE_CM_FW="$(basename ${DIR_GCC}/${CM_BUILD_TARGET}/*.bin)"
            install -m 644 ${DIR_GCC}/${CM_BUILD_TARGET}/${FILE_CM_FW} ${D}/boot/cm_${FILE_CM_FW}.${CM_BUILD_TARGET}
        done
    done

    # Install disable_cache demos (all demos in CM_DEMOS_DISABLE_CACHE)
    for CM_DEMO in ${CM_DEMOS_DISABLE_CACHE}; do
        DIR_GCC="${S}/boards/${CM_BOARD}/${CM_DEMO}/armgcc"
        # Install all build targets
        CM_BUILD_TARGET="debug"
        # Install elf
        FILE_CM_FW="$(basename ${DIR_GCC}/${CM_BUILD_TARGET}/*.elf)"
        install -m 644 ${DIR_GCC}/${CM_BUILD_TARGET}/${FILE_CM_FW} ${D}${nonarch_base_libdir}/firmware/cm_${FILE_CM_FW}.${CM_BUILD_TARGET}
    done
}

do_install:append:mx9-nxp-bsp() {
    ETHOS_U_FW="cm_ethosu_apps_rpmsg.elf.release"
    cd ${D}${nonarch_base_libdir}/firmware/
    ln -s "${ETHOS_U_FW}" ethosu_firmware
}

do_compile:imx8mp-var-dart() {
    # Configure ARMGCC
    export ARMGCC_DIR="${STAGING_DIR_NATIVE}${bindir}/cortexm-toolchain-cross-${CM_GCC}"

    # Build all board in CM_BOARD
    for cm_board in ${CM_BOARD}; do
        # Build all demos in CM_DEMOS
        for CM_DEMO in ${CM_DEMOS}; do
            compile_fw "${S}/boards/${cm_board}/${CM_DEMO}/armgcc"
        done
    done
}

do_install:imx8mp-var-dart() {
    # Install scripts
    install -d ${D}${sysconfdir}/remoteproc
    install -m 0755 ${WORKDIR}/variscite-rproc-u-boot ${D}${sysconfdir}/remoteproc
    install -m 0755 ${WORKDIR}/variscite-rproc-linux ${D}${sysconfdir}/remoteproc
    install -m 0644 ${WORKDIR}/variscite-rproc-common.sh ${D}${sysconfdir}/remoteproc
    install -m 0644 ${WORKDIR}/variscite-rproc.conf ${D}${sysconfdir}/remoteproc

    # Install firmware
    install -d ${D}${nonarch_base_libdir}/firmware/
    install -d ${D}/boot/

    # Install all demos for each board
    for cm_board in ${CM_BOARD}; do

        case "$cm_board" in
        dart_mx8mp) :
            CM_FW_SUFFIX="dart"
        ;;
        som_mx8mp) :
            CM_FW_SUFFIX="som"
        ;;
        *) :
            CM_FW_SUFFIX=""
        ;;
        esac

        # Install all demos in CM_DEMOS
        for CM_DEMO in ${CM_DEMOS}; do
            DIR_GCC="${S}/boards/${cm_board}/${CM_DEMO}/armgcc"
            # Install all build targets
            for CM_BUILD_TARGET in ${CM_BUILD_TARGETS}; do
                # Install elf
                FILE_CM_FW="$(basename ${DIR_GCC}/${CM_BUILD_TARGET}/*.elf)"
                install -m 644 ${DIR_GCC}/${CM_BUILD_TARGET}/${FILE_CM_FW} ${D}${nonarch_base_libdir}/firmware/cm_${FILE_CM_FW}.${CM_BUILD_TARGET}_${CM_FW_SUFFIX}

                # Install bin
                FILE_CM_FW="$(basename ${DIR_GCC}/${CM_BUILD_TARGET}/*.bin)"
                install -m 644 ${DIR_GCC}/${CM_BUILD_TARGET}/${FILE_CM_FW} ${D}/boot/cm_${FILE_CM_FW}.${CM_BUILD_TARGET}_${CM_FW_SUFFIX}
            done
        done
    done
}

FILES:${PN} = " \
	${sysconfdir}/remoteproc \
	${nonarch_base_libdir}/firmware \
	/boot \
"

INHIBIT_PACKAGE_STRIP = "1"
INHIBIT_PACKAGE_DEBUG_SPLIT = "1"

# Firmware files are run on M4/M7
INSANE_SKIP = "arch"
