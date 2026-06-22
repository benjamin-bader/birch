load("@protobuf//bazel:proto_library.bzl", "proto_library")
load("@protobuf//bazel:cc_proto_library.bzl", "cc_proto_library")

cc_proto_library(
    name = "cc_caps_proto",
    deps = [":caps_proto"],
    visibility = ["//visibility:public"],
)

proto_library(
    name = "caps_proto",
    srcs = ["caps.proto"],
    deps = [
        "@protobuf//:descriptor_proto",
    ],
    visibility = ["//visibility:public"],
)

proto_library (
    name = "s2s_proto",
    srcs = ["s2s.proto"],
    visibility = ["//visibility:public"],
)
