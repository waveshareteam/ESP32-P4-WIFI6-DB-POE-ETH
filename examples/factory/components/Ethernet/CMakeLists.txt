# WHOLE_ARCHIVE keeps the static app registrar that main installs from the registry.
idf_component_register(
    SRCS
        "EthernetInfo.cpp"
        "assets/img_app_ethernet.c"
    INCLUDE_DIRS "."
    REQUIRES
        brookesia_core
        board
        esp_eth
        esp_event
        esp_netif
        freertos
    WHOLE_ARCHIVE
)
