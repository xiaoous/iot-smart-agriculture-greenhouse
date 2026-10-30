# 定义源文件目录：指定LED组件的源文件所在文件夹
set(src_dirs
        send
        LED
        Myuart
        esp_recv
)

# 定义头文件搜索目录：指定LED组件的头文件所在文件夹
set(include_dirs
        send
        LED
        Myuart
        esp_recv
)

# 定义组件依赖：声明需要依赖driver组件（GPIO等驱动功能）
set(requires
        esp_wifi
        esp_netif
        esp_http_client
        nvs_flash
        freertos
        esp_event
        driver
        esp_driver_gpio
        esp_driver_uart
        esp_http_server
)

# 注册组件，将上述变量传入
idf_component_register(SRC_DIRS ${src_dirs} INCLUDE_DIRS ${include_dirs} REQUIRES ${requires})

# 为组件添加编译选项：开启快速数学运算、O3优化、关闭格式相关编译错误
component_compile_options(-ffast-math -O3 -Wno-error=format=-Wno-format)