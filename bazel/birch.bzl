def birch_copts(custom_opts = []):
  msvc_opts = [
    "-std:c++14",
  ]

  posix_opts = [
    "-std=c++14",
  ]

  darwin_opts = posix_opts + [
    # TCLAP apparently requires this to support int64_t
    "-DHAVE_LONG_LONG",
  ]

  return custom_opts + select({
    "//bazel:darwin": darwin_opts,
    "//bazel:windows": msvc_opts,
  })

def birch_linkopts(custom_linkopts):
  return custom_linkopts + select({
    "//bazel:darwin": [],
    "//conditions:default": [],
  })

def birch_cc_library(
    name,
    srcs = [],
    hdrs = [],
    copts = [],
    visibility = None,
    external_deps = None,
    tcmalloc_dep = None,
    repository = "",
    linkstamp = None,
    tags = [],
    deps = [],
    strip_include_prefix = None):
  if tcmalloc_dep:
    deps += tcmalloc_dep

  if copts == None:
    copts = []

  native.cc_library(
    name = name,
    srcs = srcs,
    hdrs = hdrs,
    copts = birch_copts() + copts,
    visibility = visibility,
    tags = tags,
    deps = deps,
  )

def birch_cc_test(
    name,
    srcs = [],
    copts = [],
    visibility = None,
    external_deps = None,
    tcmalloc_dep = None,
    repository = "",
    linkstamp = None,
    tags = [],
    deps = [],
    strip_include_prefix = None):
  if tcmalloc_dep:
    deps += tcmalloc_dep

  if copts == None:
    copts = []

  if "@gtest//:gtest_main" not in deps:
    deps += ["@gtest//:gtest_main"]

  native.cc_test(
    name = name,
    srcs = srcs,
    copts = birch_copts() + copts,
    visibility = visibility,
    tags = tags,
    deps = deps
  )

def birch_cc_binary(
    name,
    srcs = [],
    copts = [],
    visibility = None,
    external_deps = None,
    tcmalloc_dep = None,
    repository = "",
    linkstamp = None,
    tags = [],
    deps = [],
    strip_include_prefix = None):
  if tcmalloc_dep:
    deps += tcmalloc_dep

  if copts == None:
    copts = []

  native.cc_binary(
    name = name,
    srcs = srcs,
    copts = birch_copts() + copts,
    visibility = visibility,
    tags = tags,
    deps = deps
  )
  