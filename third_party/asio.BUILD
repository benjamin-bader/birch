licenses(["notice"])

cc_library(
    name = "asio",
    srcs = [],
    hdrs = glob(["**/*.hpp", "**/*.ipp"]),
    visibility = ["//visibility:public"],
    defines = [
        "ASIO_STANDALONE",
        "ASIO_HAS_MOVE",
        "ASIO_HAS_STD_CHRONO",
        "ASIO_HAS_STD_SYSTEM_ERROR",
        "ASIO_HAS_CONSTEXPR", # TBD: Is this actually true these days?
    ]
)