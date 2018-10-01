config_setting (
  name = "darwin",
  constraint_values = [ "@bazel_tools//platforms:osx" ],
  visibility = ["//visibility:public"],
)

config_setting (
  name = "windows",
  constraint_values = [ "@bazel_tools//platforms:windows" ],
  visibility = ["//visibility:public"],
)

load('//:vars.bzl', 'COPTS')

cc_binary(
    name = "birch",
    srcs = ["main.cc"],
    deps = [
        "//server",
        "@tclap",
    ],
    copts = COPTS,
)
