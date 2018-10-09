load("//bazel:birch.bzl", "birch_cc_binary")

birch_cc_binary(
    name = "birch",
    srcs = ["main.cc"],
    deps = [
        "//server",
        "@tclap",
        "@spdlog",
        "@absl//absl/strings",
    ],
)
