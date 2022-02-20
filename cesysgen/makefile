!if 0
Copyright (c) Microsoft Corporation.  All rights reserved.
!endif
!if 0
Use of this sample source code is subject to the terms of the Microsoft
license agreement under which you licensed this sample source code. If
you did not accept the terms of the license agreement, you are not
authorized to use this sample source code. For the terms of the license,
please see the license agreement between you and Microsoft or, if applicable,
see the LICENSE.RTF on your install media or the root of your tools installation.
THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
!endif

# This file defines custom build actions that occur before (preproc) and
# after (postproc) the BSP compiles and links.  It is responsible for
# performing cefilter on the platform.bib and platform.reg files, which
# is achieved through the cefilter_files rule below (the rule is
# implemented in the CeSysgenPlatform.mak.

#
# Include platform shared SYSGEN makefile
#
!INCLUDE $(_WINCEROOT)\public\common\cesysgen\CeSysgenPlatform.mak

# Rules run before the BSP builds
# Filter the CESYSGEN tags in the platform.bib and .reg files.
preproc: cefilter_files

# Rules run after the BSP builds
postproc:
