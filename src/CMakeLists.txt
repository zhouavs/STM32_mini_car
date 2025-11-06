# 自动收集 src 目录下所有 C 文件
file(GLOB_RECURSE ALL_C_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

# 创建 src 目录的对象库
add_library(user_src OBJECT ${ALL_C_SOURCES})

# -fshort-enums 表示 enum 类型使用最小字节数存储
# 只对 user_src 加 -fshort-enums
target_compile_options(user_src PRIVATE -fshort-enums)

# 添加所有文件夹为 include 路径
target_include_directories(user_src PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(user_src PRIVATE stm32cubemx)

target_link_libraries(${CMAKE_PROJECT_NAME} user_src)
