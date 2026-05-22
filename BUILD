load("@rules_cc//cc:defs.bzl", "cc_binary")
load("@birch_version//:version.bzl", "VERSION")

cc_binary(
    name = "birch",
    srcs = ["main.cc"],
    deps = [
        "//config:config",
        "//config:file_config_data_source",
        "//config:toml_config",
        "//server",
        "//util:file_watcher",
        "@abseil-cpp//absl/flags:flag",
        "@abseil-cpp//absl/flags:parse",
        "@abseil-cpp//absl/flags:usage",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:flags",
        "@abseil-cpp//absl/log:initialize",
        "@abseil-cpp//absl/strings",
    ],
    defines = [
        "VERSION=\\\"{}\\\"".format(VERSION),
    ],
)
