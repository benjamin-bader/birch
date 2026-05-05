package(default_visibility = ["//visibility:public"])

config_setting(
    name = "linux_x64",
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
    ],
)

config_setting(
    name = "linux_x86",
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_32",
    ],
)

config_setting(
    name = "darwin",
    constraint_values = ["@platforms//os:macos"],
)

config_setting(
    name = "windows",
    constraint_values = [
        "@platforms//os:windows",
        "@platforms//cpu:x86_64",
    ],
)