load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "config_path",
    hdrs = ["ConfigPath.h"],
    srcs = ["ConfigPath.cc"],
    deps = [
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "iconfig_source",
    hdrs = ["IConfigSource.h"],
    deps = [
        "@abseil-cpp//absl/status:statusor",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "file_config_data_source",
    hdrs = ["FileConfigDataSource.h"],
    srcs = ["FileConfigDataSource.cc"],
    deps = [
        ":iconfig_source",
        "//util:file_watcher",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "file_config_data_source_test",
    srcs = ["FileConfigDataSourceTest.cc"],
    deps = [
        ":file_config_data_source",
        "//tools:test_main",
        "@abseil-cpp//absl/status",
    ],
    size = "small",
)

cc_library(
    name = "config",
    hdrs = ["IConfig.h"],
    deps = [
        ":config_path",
        ":iconfig_source",
        "@abseil-cpp//absl/container:flat_hash_map",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "toml_config",
    hdrs = ["TomlConfig.h"],
    srcs = ["TomlConfig.cc"],
    deps = [
        ":config_path",
        ":config",
        ":iconfig_source",
        "@abseil-cpp//absl/container:btree",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
        "@tomlplusplus//:tomlplusplus",
    ],
    visibility = ["//visibility:public"],
)

cc_test(
    name = "toml_config_test",
    srcs = ["TomlConfigTest.cc"],
    deps = [
        ":toml_config",
        ":file_config_data_source",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
    size = "small",
)
