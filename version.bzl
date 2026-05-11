"""
Rule and extension to get the version from the root module.

Why is Bazel like this.
"""

def _version_repo_impl(repository_ctx):
    # repository_ctx.attr.version is passed from the extension below
    repository_ctx.file("version.bzl", "VERSION = \"{}\"".format(repository_ctx.attr.version))
    repository_ctx.file("BUILD", "")

version_repo = repository_rule(
    implementation = _version_repo_impl,
    attrs = {"version": attr.string()},
)

def _version_extension_impl(module_ctx):
    # module_ctx.modules[0] is the root module
    root_module = module_ctx.modules[0]
    version_repo(name = "birch_version", version = root_module.version)

version_extension = module_extension(implementation = _version_extension_impl)
