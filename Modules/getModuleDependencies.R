args <- commandArgs(TRUE)

# meant to be called via system or system2
#
# args should have the form
#
# library path
# repository
# tempfile
# paths to jasp modules

# example
# args <- c(
#   build/R/pkgdepends_library,
#   "https://cloud.r-project.org",
#   tempfile(),
#   "jasp-desktop/Modules/jaspTTests",
#   "jasp-desktop/Engine/jaspBase",
#   "jasp-desktop/Engine/jaspGraphs"
# )

stopifnot(length(args) >= 4L)

.libPaths(args[1])
print(.libPaths())
options(repos = args[2])

tempPath    <- args[3]
jaspModules <- args[4:length(args)]

print("a")
pd <- pkgdepends::pkg_deps$new(jaspModules)
print("b")
pd$solve()
print("b2")
pd$draw()
print("c")
sol <- pd$get_solution()
print("d")
dat <- sol$data

# this could be expanded but we currently do not support other repos anyway...
fromRepository <- which(dat$type == "standard")
fromGitHub     <- which(dat$type == "github")

recordsFromRepository <- setNames(lapply(fromRepository, function(i) {
  list(Package = dat$package[i], Version = dat$version[i], Source = "Repository")
}), dat$package[fromRepository])

recordsFromGithub <- setNames(lapply(fromGitHub, function(i) {
  list(
    Package        = dat$package[i],
    Version        = dat$version[i],
    Source         = "GitHub",
    RemoteUsername = dat$remote[[i]]$username,
    RemoteRepo     = dat$remote[[i]]$repo
  )
}), dat$package[fromGitHub])

combinedRecords <- c(recordsFromGithub, recordsFromRepository)

saveRDS(combinedRecords, file = tempPath)
