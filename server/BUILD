load("//bazel:birch.bzl", "birch_cc_library", "birch_cc_test")

birch_cc_library(
    name = "message",
    srcs = ["Message.cc"],
    hdrs = ["Message.h"],
)

birch_cc_library(
    name = "message_parser",
    srcs = ["MessageParser.cc"],
    hdrs = ["MessageParser.h"],
    deps = [
        "message",
        "@asio",
    ],
)

birch_cc_test(
    name = "message_parser_test",
    srcs = ["MessageParserTest.cc"],
    deps = ["message_parser"],
)

birch_cc_library(
    name = "channel",
    srcs = ["Channel.cc"],
    hdrs = ["Channel.h"],
)

birch_cc_test(
    name = "channel_test",
    srcs = ["ChannelTest.cc"],
    deps = ["channel"]
)

birch_cc_library(
    name = "connection",
    srcs = ["Connection.cc"],
    hdrs = ["Connection.h"],
    deps = [
        "@asio"
    ],
)

birch_cc_library(
    name = "user",
    srcs = ["User.cc"],
    hdrs = ["User.h"],
)

birch_cc_library(
    name = "server",
    srcs = ["Server.cc"],
    hdrs = ["Server.h"],
    deps = [
        "channel",
        "message_parser",
        "connection",
        "user",
        "@asio//:asio",
        "@boringssl//:ssl",
    ],
    visibility = ["//visibility:public"],
)
