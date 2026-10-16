idf_component_register(
    SRCS
        "EthernetInfo.cpp"
        "assets/img_app_ethernet.c"
    INCLUDE_DIRS
        "."
    REQUIRES brookesia_core ethernet_init esp_eth esp_event esp_netif freertos
)
