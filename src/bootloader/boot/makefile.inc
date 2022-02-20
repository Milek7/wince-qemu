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
!if 0
In this file, we'll call romimage to create our bootloader image (the output will
be "boot.bin") as part of the normal build process.
!endif

# Take the boot.exe that resulted from the standard build and generate boot.bin from it.
$(SG_OUTPUT_ROOT)\platform\$(_TGTPLAT)\target\$(_CPUINDPATH)\boot.bin:
# generate boot.bin
    romimage $(ROMIMAGE_FLAGS) boot.bib
!IF "$(WINCEREL)"=="1"
# copy the output to the release directory
    copy $(SG_OUTPUT_ROOT)\platform\$(_TGTPLAT)\target\$(_CPUDEPPATH)\boot.* $(_FLATRELEASEDIR)
!ENDIF

