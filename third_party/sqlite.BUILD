load("@rules_cc//cc:defs.bzl", "cc_library")

licenses(["unencumbered"])

SQLITE_COPTS = [
    "-DSQLITE_ENABLE_JSON1",
    "-DHAVE_DECL_STRERROR_R=1",
    "-DHAVE_STDINT_H=1",
    "-DHAVE_INTTYPES_H=1",
    "-D_FILE_OFFSET_BITS=64",
    "-D_REENTRANT=1",
]

cc_library(
    name = "sqlite",
    srcs = ["sqlite3.c"],
    hdrs = [
        "sqlite3.h",
        "sqlite3ext.h",
    ],
    copts = SQLITE_COPTS,
    defines = ["SQLITE_OMIT_DEPRECATED"],
    linkopts = ["-ldl", "-lpthread"],
    visibility = ["//visibility:public"],
)