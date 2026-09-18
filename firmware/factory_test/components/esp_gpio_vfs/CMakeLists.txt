idf_component_register(SRCS "esp_gpio_vfs.c"
                       INCLUDE_DIRS "include"
                       REQUIRES vfs
                       PRIV_REQUIRES esp_driver_gpio)
