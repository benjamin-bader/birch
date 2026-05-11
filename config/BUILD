load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "iconfig_source",
    hdrs = ["IConfigSource.h"],
    deps = [
        "@abseil-cpp//absl/status:statusor",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "toml_config_source",
    hdrs = ["TomlConfigSource.h"],
    srcs = ["TomlConfigSource.cc"],
    deps = [
        ":iconfig_source",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/strings",
        "@tomlplusplus//:tomlplusplus",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "toml_config_source_test",
    srcs = ["TomlConfigSourceTest.cc"],
    deps = [
        "//tools:test_main",
        ":toml_config_source",
        "@abseil-cpp//absl/status",
        "@googletest//:gtest",
    ],
    size = "small",
)

cc_library(
    name = "value_source_iface",
    hdrs = ["IValueSource.h"],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "value_sources",
    hdrs = ["ValueSources.h"],
    srcs = ["ValueSources.cc"],
    deps = [
        ":value_source_iface",
    ],
    visibility = ["//visibility:private"],
)

cc_test(
    name = "value_sources_test",
    srcs = ["ValueSourcesTest.cc"],
    deps = [
        ":value_sources",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
    size = "small",
)

cc_library(
    name = "config",
    hdrs = ["Config.h"],
    srcs = ["Config.cc"],
    deps = [
        ":iconfig_source",
        ":value_source_iface",
        ":value_sources",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/strings",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "config_test",
    srcs = ["ConfigTest.cc"],
    deps = [
        ":config",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
    size = "small",
)
