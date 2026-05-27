#stupid little script that installs modules listed in remote-bundles.json and bundles located in Modules/local
#Logs what it has installed/downloaded and does not reinstall stuff if not necesarry.

MODULES_DIR <- file.path("/home/virtuoos/Broncode/jasp-desktop", "Modules")
LOCAL_MODULES_DIR <- file.path("/home/virtuoos/Broncode/jasp-desktop", "Modules", "local")
BUNDLE_DOWNLOAD_DIR <- file.path("/home/virtuoos/Broncode/jasp-desktop", "Modules", "downloads")

if(FALSE) {
    print("Linking to pre-build modules dir")
    unlink("/home/virtuoos/Broncode/jasp-desktop/Modules", recursive=TRUE)
    file.symlink("/app/Modules/", "/home/virtuoos/Broncode/jasp-desktop/Modules")
}

getPlatform <- function() {
    info <- Sys.info()
    os <- ""
    switch(info[['sysname']],
        Windows = {os <- "Windows-x86_64"},
        Linux   = {os <- "Linux"},
        Darwin  = {os <- if(info[['machine']] == "arm64") "MacOS-arm64" else "MacOS-x86_64"}
    )
    os
}
mkdir <- function(paths) {
  for (path in paths)
    if (!dir.exists(path))
      dir.create(path, recursive = TRUE)
}
mkdir(c(MODULES_DIR, LOCAL_MODULES_DIR, BUNDLE_DOWNLOAD_DIR))


.libPaths("/home/virtuoos/Broncode/jasp-desktop/Modules/Tools/jaspModuleBundleManager_library")
library("jaspModuleBundleManager")
library("tools")

#check which bundles need downloading
remoteBundles <- rjson::fromJSON(file="/home/virtuoos/Broncode/jasp-desktop/Modules/remote-bundles.json")[[getPlatform()]]
remoteBundleURLS <- unlist(sapply(remoteBundles, function(x){x$url}))

toInstallRemote <- c()
toInstallLocal <- c()
installed <- c()

if(length(remoteBundleURLS) > 0) {
    remoteBundleDownloaded <- c()
    if(file.exists("/home/virtuoos/Broncode/jasp-desktop/Modules/bundles-downloaded.txt")) {
        tryCatch({
            remoteBundleDownloaded <- read.delim("/home/virtuoos/Broncode/jasp-desktop/Modules/bundles-downloaded.txt", header = FALSE)[[1]]
            remoteBundleDownloaded <- remoteBundleDownloaded[file.exists(file.path(BUNDLE_DOWNLOAD_DIR, basename(remoteBundleDownloaded)))]
        }, error = function(e) { remoteBundleDownloaded <- c() })
    }
    toDownload <- remoteBundles[!remoteBundleURLS %in% remoteBundleDownloaded]

    #download and write a file with all our successes
    downloaded <- c()
    if(length(toDownload)) {
        cat("We will download: ", paste(toDownload, sep=' '), '\n')
        download <- function(bundle) {
            req <- tryCatch({
                path <- file.path(BUNDLE_DOWNLOAD_DIR, basename(bundle$url))
                res <- curl::curl_fetch_disk(bundle$url, path)
                if(sha256sum(files=path) != bundle$checksum) {
                    file.remove(path)
                    warning(paste0("Checksum of: ", bundle$url, "Does not match. Skipping"))
                    res <- list(status_code=404)
                }
                res 
            }, error = function(e) { print(e); list(status_code=404) })
            if(req$status_code != 200)
                return(FALSE)
            TRUE
        }
        res <- sapply(toDownload, download)
        success <- unlist(sapply(toDownload[res], function(x){x$url}))
        downloaded <- unique(c(remoteBundleDownloaded[remoteBundleDownloaded %in% remoteBundleURLS], success))
        write(downloaded, "/home/virtuoos/Broncode/jasp-desktop/Modules/bundles-downloaded.txt")

        if(!all(res)) {
            toDownload <- toDownload[!res]
            cat("Unable to download the following bundles: ", paste(toDownload, sep = ' '), '\n')
            quit(status=66)
        }

        #clean up the download folder a bit by removing all bundles we where not instructed to download
        files <- list.files(BUNDLE_DOWNLOAD_DIR)
        leftover <- files[! files %in% basename(downloaded)]
        file.remove(file.path(BUNDLE_DOWNLOAD_DIR, leftover))
    }


    # use the jaspModuleBundleManager to install the downloaded bundles and the ones in the local overwrite folder
    toInstallRemoteTotal <- remoteBundleURLS[file.exists(file.path(BUNDLE_DOWNLOAD_DIR, basename(remoteBundleURLS)))]
    prevInstalledRemote <- c()
    if(file.exists("/home/virtuoos/Broncode/jasp-desktop/Modules/remote-bundles-installed.txt")) {
        tryCatch({
            prevInstalledRemote <- read.delim("/home/virtuoos/Broncode/jasp-desktop/Modules/remote-bundles-installed.txt", header = FALSE)[[1]]
        }, error = function(e) { prevInstalledRemote <- c() })
    }
    toInstallRemote <- toInstallRemoteTotal[!toInstallRemoteTotal %in% prevInstalledRemote]

    if(length(toInstallRemote) > 0) {
        installRemote <- function(remote) {
            bundlePath <- file.path(BUNDLE_DOWNLOAD_DIR, basename(remote))
            cat("Installing: ", bundlePath, "\n")
            jaspModuleBundleManager::installJaspModuleBundle("/home/virtuoos/Broncode/jasp-desktop/Modules", bundlePath)
        }
        res <- sapply(toInstallRemote, installRemote)
        installed <- c(installed, res)
        write(toInstallRemoteTotal, "/home/virtuoos/Broncode/jasp-desktop/Modules/remote-bundles-installed.txt", ncolumns=1)
    }
}

localBundles <- file.path(LOCAL_MODULES_DIR, list.files(LOCAL_MODULES_DIR, pattern='*.JASPModule'))
if(length(localBundles) > 0) {
    prevInstalledLocal <- c()
    if(file.exists("/home/virtuoos/Broncode/jasp-desktop/Modules/local-bundles-installed.txt")) {
        tryCatch({
            prevInstalledLocal <- read.delim("/home/virtuoos/Broncode/jasp-desktop/Modules/local-bundles-installed.txt", header = FALSE)
        }, error = function(e) { prevInstalledLocal <- c() })
    }
    newOrChanged <- function(bundle){
        index <- match(bundle, prevInstalledLocal[[1]])
        if(is.na(index)) TRUE #not installed before
        else if(prevInstalledLocal[[2]][index] != toString(file.mtime(bundle))) TRUE #changed
        else FALSE
    }
    res <- sapply(localBundles, newOrChanged)
    toInstallLocal <- localBundles[res]

    #install all new or changed local bundles
    if(length(toInstallLocal) > 0) {
        installLocal <- function(bundle) {
            cat("Installing: ", bundle, "\n")
            jaspModuleBundleManager::installJaspModuleBundle("/home/virtuoos/Broncode/jasp-desktop/Modules", bundle)
        }
        res <- sapply(toInstallLocal, installLocal)
        installed <- c(installed, res)
        mtimes = sapply(file.mtime(localBundles), toString)
        write(c(rbind(localBundles, mtimes)), "/home/virtuoos/Broncode/jasp-desktop/Modules/local-bundles-installed.txt", sep='\t', ncolumns=2)
    }
}
