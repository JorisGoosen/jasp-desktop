# Generated from install-tools.R.in
#

JASP_MODULE_BUNDLE_MANAGER_LIBRARY 	<- "/home/virtuoos/Broncode/jasp-desktop/Modules/Tools/jaspModuleBundleManager_library"
RENV_LIBRARY                		    <- "/home/virtuoos/Broncode/jasp-desktop/_cache/R/renv_library"
R_CPP_INCLUDES_LIBRARY              <- "/home/virtuoos/Broncode/jasp-desktop/Modules/Tools/R_cpp_includes_library"

ENGINE                 <- file.path("/home/virtuoos/Broncode/jasp-desktop", "Engine")
MODULES                <- file.path("/home/virtuoos/Broncode/jasp-desktop", "Modules")

mkdir <- function(paths) {
  for (path in paths)
    if (!dir.exists(path))
      dir.create(path, recursive = TRUE)
}
mkdir(c(R_CPP_INCLUDES_LIBRARY, JASP_MODULE_BUNDLE_MANAGER_LIBRARY))

.libPaths(RENV_LIBRARY)
options(repos = c(CRAN = "https://packagemanager.posit.co/cran/latest"))
options(INSTALL_opts = "--no-test-load")
Sys.setenv(CPPFLAGS = "-DENABLE_LEGACY_NONAPI")
cat("Restoring Rcpp & RInside\n")
renv::restore(
  library  = R_CPP_INCLUDES_LIBRARY,
  lockfile = file.path(MODULES, "Rcpp_RInside.lock"),
  clean    = TRUE
)

# jaspModuleBundleManager and its dependencies are not critical for C++ build
# and may take very long to compile (e.g., stringi/ICU) or fail on newer toolchains
cat("Skipping jaspModuleBundleManager deps restore (non-critical)\n")
