/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is
 * Mozilla Corporation.
 * Portions created by the Initial Developer are Copyright (C) 2007
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Pelle Johnsen <pjohnsen@mozilla.com>
 *   Dave Camp <dcamp@mozilla.com>
 *   Tobias Hunger <tobias.hunger@gmail.com>
 *   Tatiana Meshkova <tanya.meshkova@gmail.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "EmbeddingSetup.h"
#include "embed.h"

// CRT headers
#include <iostream>
#include <string>
using namespace std;

#include "nsXULAppAPI.h"
#include "nsXPCOMGlue.h"
#include "nsCOMPtr.h"
#include "nsStringAPI.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsProfileDirServiceProvider.h"

#include "nsILocalFile.h"

#ifdef WIN32
  //TODO: make this file fully X platform
#  include <windows.h>
#  undef MAX_PATH
#  define MAX_PATH _MAX_PATH
#else
#  include <unistd.h>
#  include <string.h>
#  ifndef PATH_MAX
#    define PATH_MAX 1024
#  endif
#  define MAX_PATH PATH_MAX
#endif

XRE_InitEmbedding2Type XRE_InitEmbedding2 = 0;
XRE_TermEmbeddingType XRE_TermEmbedding = 0;
XRE_NotifyProfileType XRE_NotifyProfile = 0;
XRE_LockProfileDirectoryType XRE_LockProfileDirectory = 0;

static int gInitCount = 0;

// ------------------------------------------------------------------------

nsIDirectoryServiceProvider *sAppFileLocProvider = 0;
nsCOMPtr<nsIFile> sProfileDir = 0;
nsISupports * sProfileLock = 0;

class MozEmbedDirectoryProvider : public nsIDirectoryServiceProvider2
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER
    NS_DECL_NSIDIRECTORYSERVICEPROVIDER2
};

static const MozEmbedDirectoryProvider kDirectoryProvider;

NS_IMPL_QUERY_INTERFACE2(MozEmbedDirectoryProvider,
                         nsIDirectoryServiceProvider,
                         nsIDirectoryServiceProvider2)

NS_IMETHODIMP_(nsrefcnt)
MozEmbedDirectoryProvider::AddRef()
{
    return 1;
}

NS_IMETHODIMP_(nsrefcnt)
MozEmbedDirectoryProvider::Release()
{
    return 1;
}

NS_IMETHODIMP
MozEmbedDirectoryProvider::GetFile(const char *aKey, bool *aPersist,
                                   nsIFile* *aResult)
{
    if (sAppFileLocProvider) {
        nsresult rv = sAppFileLocProvider->GetFile(aKey, aPersist, aResult);
        if (NS_SUCCEEDED(rv))
            return rv;
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_USER_PROFILE_50_DIR)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_PROFILE_DIR_STARTUP)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    if (sProfileDir && !strcmp(aKey, NS_APP_CACHE_PARENT_DIR)) {
        *aPersist = PR_TRUE;
        return sProfileDir->Clone(aResult);
    }

    return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
MozEmbedDirectoryProvider::GetFiles(const char *aKey,
                                    nsISimpleEnumerator* *aResult)
{
    nsCOMPtr<nsIDirectoryServiceProvider2>
        dp2(do_QueryInterface(sAppFileLocProvider));

    if (!dp2)
        return NS_ERROR_FAILURE;

    return dp2->GetFiles(aKey, aResult);
}

nsresult InitEmbedding(const char* aProfilePath)
{
    nsresult rv;

    ++gInitCount;
    if (gInitCount > 1)
        return NS_OK;

#if 0
    // Find the GRE (xul shared lib). We are only using frozen interfaces, so we
    // should be compatible all the way up to (but not including) mozilla 2.0
    static const GREVersionRange vr = {
        "1.9a1",
        PR_TRUE,
        "2.0",
        PR_FALSE
    };
    // find xpcom shared lib (uses GRE_HOME env var if set)
    char temp[MAX_PATH];
    rv = GRE_GetGREPathWithProperties(&vr, 1, nullptr, 0, temp, sizeof(temp));
    string xpcomPath(temp);
#else

#if __WORDSIZE == 64
    string xpcomPath("/usr/lib64/xulrunner/libxpcom.so");
#else
    string xpcomPath("/usr/lib/xulrunner/libxpcom.so");
#endif

    const char* greHome = getenv("GRE_HOME");
    printf("GRE_HOME: %s\n", greHome);
    if (greHome) {
	xpcomPath = greHome;
	xpcomPath += "/libxpcom.so";
    }

#endif

    cout << "xpcom: " << xpcomPath << endl;

    if (NS_FAILED(rv)) {
        cerr << "Unable to find GRE, try setting GRE_HOME." << endl;
        return NS_ERROR_FAILURE;
    }

    // start the glue, i.e. load and link against xpcom shared lib
    rv = XPCOMGlueStartup(xpcomPath.c_str());
    if (NS_FAILED(rv)) {
        cerr << "Could not start XPCOM glue." << endl;
        return NS_ERROR_FAILURE;
    }

    // get rid of the bogus TLS warnings
    NS_LogInit();

    // load XUL functions
    nsDynamicFunctionLoad nsFuncs[] = {
            {"XRE_InitEmbedding2", (NSFuncPtr*)&XRE_InitEmbedding2},
            {"XRE_TermEmbedding", (NSFuncPtr*)&XRE_TermEmbedding},
            {"XRE_NotifyProfile", (NSFuncPtr*)&XRE_NotifyProfile},
            {"XRE_LockProfileDirectory", (NSFuncPtr*)&XRE_LockProfileDirectory},
            {0, 0}
    };

    rv = XPCOMGlueLoadXULFunctions(nsFuncs);
    if (NS_FAILED(rv)) {
        cerr << "Could not load XUL functions." << endl;
        return NS_ERROR_FAILURE;
    }

    // strip the filename from xpcom so we have the dir instead
    size_t lastslash = xpcomPath.find_last_of("/\\");
    if (lastslash == string::npos) {
        cerr << "Invalid path to xpcom:" << xpcomPath << "." << endl;
        return NS_ERROR_FAILURE;
    }
    string xpcomDir = xpcomPath.substr(0, lastslash);

    // create nsILocalFile pointing to xpcomDir
    nsCOMPtr<nsIFile> xuldir;
    rv = NS_NewNativeLocalFile(nsCString(xpcomDir.c_str()), PR_FALSE,
                               getter_AddRefs(xuldir));
    if (NS_FAILED(rv)) {
        cerr << "Unable to create nsILocalFile for xuldir " << xpcomDir
             << "." << endl;
        return NS_ERROR_FAILURE;
    }

    // create nsIFile pointing to appdir
    char self[MAX_PATH];
#ifdef WIN32
    GetModuleFileNameA(GetModuleHandle(NULL), self, sizeof(self));
#else
    // TODO: works on linux, need solution for unices which do not support this
    ssize_t len;
    if ((len = readlink("/proc/self/exe", self, sizeof(self)-1)) != -1)
        self[len] = '\0';
#endif
    string selfPath(self);
    lastslash = selfPath.find_last_of("/\\");
    if (lastslash == string::npos) {
        cerr << "Invalid module filename: " << self << "." << endl;
        return NS_ERROR_FAILURE;
    }

    selfPath = selfPath.substr(0, lastslash);

    nsCOMPtr<nsIFile> appdir;
    rv = NS_NewNativeLocalFile(nsCString(selfPath.c_str()), PR_FALSE,
                               getter_AddRefs(appdir));
    if (NS_FAILED(rv)) {
        cerr << "Unable to create nsILocalFile for appdir." << endl;
        return NS_ERROR_FAILURE;
    }

    // setup profile dir
    nsCString pr(aProfilePath);
    if (!pr.IsEmpty()) {
        rv = NS_NewNativeLocalFile(pr, PR_FALSE,
                                   getter_AddRefs(sProfileDir));
        NS_ENSURE_SUCCESS(rv, rv);
    } else {
        // for now use a subdir under appdir
        nsCOMPtr<nsIFile> profFile;
        rv = appdir->Clone(getter_AddRefs(profFile));
        NS_ENSURE_SUCCESS(rv, rv);
        sProfileDir = do_QueryInterface(profFile);
        sProfileDir->AppendNative(NS_LITERAL_CSTRING("mozembed"));
    }

    // create dir if needed
    bool dirExists;
    rv = sProfileDir->Exists(&dirExists);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!dirExists) {
        sProfileDir->Create(nsIFile::DIRECTORY_TYPE, 0700);
    }

    // Lock profile directory
    if (sProfileDir && !sProfileLock) {
        rv = XRE_LockProfileDirectory(sProfileDir, &sProfileLock);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    // init embedding
    rv = XRE_InitEmbedding2(xuldir, appdir,
                           const_cast<MozEmbedDirectoryProvider*>(&kDirectoryProvider));
    if (NS_FAILED(rv)) {
        cerr << "XRE_InitEmbedding2 failed." << endl;
        return NS_ERROR_FAILURE;
    }

    // initialize profile:
    XRE_NotifyProfile();

    NS_LogTerm();

    return NS_OK;
}

nsresult TermEmbedding()
{
    --gInitCount;
    if (gInitCount > 0)
        return NS_OK;

    nsresult rv;

    // get rid of the bogus TLS warnings
    NS_LogInit();

    // terminate embedding
    if (!XRE_TermEmbedding) {
        cerr << "XRE_TermEmbedding not set." << endl;
        return NS_ERROR_ABORT;
    }

    XRE_TermEmbedding();

    // make sure this is freed before shutting down xpcom
    NS_IF_RELEASE(sProfileLock);
    sProfileDir = 0;

    // shutdown xpcom
    rv = XPCOMGlueShutdown();
    if (NS_FAILED(rv)) {
        cerr << "Could not shutdown XPCOM glue." << endl;
        return rv;
    }

    NS_LogTerm();

    return NS_OK;
}
