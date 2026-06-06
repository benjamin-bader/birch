load("@rules_cc//cc:defs.bzl", "cc_library", "cc_test")

cc_library(
    name = "message",
    srcs = ["Message.cc"],
    hdrs = ["Message.h"],
    deps = [
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/container:linked_hash_map",
    ],
)

cc_test(
    name = "message_test",
    size = "small",
    srcs = ["MessageTest.cc"],
    deps = [
        ":message",
        "//tools:test_main",
        "@abseil-cpp//absl/strings",
        "@abseil-cpp//absl/strings:str_format",
    ],
)

cc_library(
    name = "message_parser",
    srcs = ["MessageParser.cc"],
    hdrs = ["MessageParser.h"],
    deps = [
        "message",
        "@abseil-cpp//absl/strings",
        "@asio",
    ],
)

cc_test(
    name = "message_parser_test",
    size = "small",
    srcs = ["MessageParserTest.cc"],
    deps = [
        "message_parser",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "message_serializer",
    srcs = ["MessageSerializer.cc"],
    hdrs = ["MessageSerializer.h"],
    deps = [
        ":message",
    ],
)

cc_library(
    name = "channel",
    srcs = ["Channel.cc"],
    hdrs = ["Channel.h"],
)

cc_test(
    name = "channel_test",
    size = "small",
    srcs = ["ChannelTest.cc"],
    deps = [
        ":channel",
        "//tools:test_main",
        "@googletest//:gtest",
    ],
)

cc_library(
    name = "connection",
    srcs = ["Connection.cc"],
    hdrs = ["Connection.h"],
    deps = [
        ":message",
        "@abseil-cpp//absl/status",
    ],
)

cc_library(
    name = "asio_connection",
    srcs = ["AsioConnection.cc"],
    hdrs = ["AsioConnection.h"],
    deps = [
        ":connection",
        ":message",
        ":message_parser",
        ":message_serializer",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@asio",
    ],
)

cc_library(
    name = "connection_list",
    srcs = ["ConnectionList.cc"],
    hdrs = ["ConnectionList.h"],
    deps = [
        ":connection",
        ":dispatcher",
    ],
)

cc_library(
    name = "session",
    srcs = ["Session.cc"],
    hdrs = ["Session.h"],
    deps = [
        ":connection",
        ":message",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@abseil-cpp//absl/status",
        "@abseil-cpp//absl/status:statusor",
        "@abseil-cpp//absl/strings",
    ],
)

cc_library(
    name = "dispatcher",
    srcs = ["Dispatcher.cc"],
    hdrs = ["Dispatcher.h"],
    deps = [
        ":connection",
        ":message",
        "@abseil-cpp//absl/log",
    ],
)

cc_library(
    name = "user",
    srcs = ["User.cc"],
    hdrs = ["User.h"],
)

cc_library(
    name = "server",
    srcs = ["Server.cc"],
    hdrs = ["Server.h"],
    visibility = ["//visibility:public"],
    deps = [
        ":asio_connection",
        ":channel",
        ":connection",
        ":message_parser",
        ":user",
        "//config",
        "@abseil-cpp//absl/container:flat_hash_map",
        "@abseil-cpp//absl/log",
        "@abseil-cpp//absl/log:check",
        "@asio",
    ],
)
