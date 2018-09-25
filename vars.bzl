# Common C++ compiler flags

ASIO_COPTS = [
    "-DASIO_STANDALONE",
    "-DASIO_HAS_MOVE",
    "-DASIO_HAS_STD_CHRONO",
    "-DASIO_HAS_STD_SYSTEM_ERROR",
]

SUPPRESSED_WARNINGS = [
    "-Wno-unused-local-typedef",
]

COPTS = ["-std=c++14",] + ASIO_COPTS + SUPPRESSED_WARNINGS