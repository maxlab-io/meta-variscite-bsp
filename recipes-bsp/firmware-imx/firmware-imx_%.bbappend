do_install_append() {
	rm -rf ${D}${nonarch_base_libdir}/firmware/bcm
}

do_install_prepend_mx8qm() {
	install -d ${D}/boot
	install -m 0644 firmware/hdmi/cadence/hdmitxfw.bin ${D}/boot
	install -m 0644 firmware/hdmi/cadence/hdmirxfw.bin ${D}/boot
	install -m 0644 firmware/hdmi/cadence/dpfw.bin ${D}/boot
}

PACKAGES =+ "${PN}-hdmi"

FILES_${PN}_remove += "${nonarch_base_libdir}/firmware/bcm/*"
FILES_${PN}-hdmi = "/boot/"

