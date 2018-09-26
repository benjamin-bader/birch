# Common C++ compiler flags

ASIO_COPTS = select({
    "//:darwin": [
        "-DASIO_STANDALONE",
        "-DASIO_HAS_MOVE",
        "-DASIO_HAS_STD_CHRONO",
        "-DASIO_HAS_STD_SYSTEM_ERROR",
    ],

    "//:windows": [
        "/DASIO_STANDALONE",
        "/DASIO_HAS_MOVE",
        "/DASIO_HAS_STD_CHRONO",
        "/DASIO_HAS_STD_SYSTEM_ERROR",
    ],
})

SUPPRESSED_WARNINGS = select({
    "//:darwin": [
        "-Wno-unused-local-typedef",
    ],

    "//:windows": [],
})

CPP_FLAGS = select({
    "//:darwin": [
        "-std=c++14",
    ],

    "//:windows": [
        "/std:c++14",
    ]
})

COPTS = CPP_FLAGS + ASIO_COPTS + SUPPRESSED_WARNINGS