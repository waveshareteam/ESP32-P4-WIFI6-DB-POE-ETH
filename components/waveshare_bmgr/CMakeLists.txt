# Sources under the board directories (ethernet.c, lcd_brightness.c, ...) are
# compiled by the generated gen_bmgr_codes component, not by this one. Their
# component dependencies are declared per device in board_devices.yaml.
# This component only carries the Kconfig menu, include/ and the sources the
# amends attach to gen_bmgr_codes.
idf_component_register()
