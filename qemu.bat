@REM
@REM Copyright (c) Microsoft Corporation.  All rights reserved.
@REM
@REM
@REM Use of this sample source code is subject to the terms of the Microsoft
@REM license agreement under which you licensed this sample source code. If
@REM you did not accept the terms of the license agreement, you are not
@REM authorized to use this sample source code. For the terms of the license,
@REM please see the license agreement between you and Microsoft or, if applicable,
@REM see the LICENSE.RTF on your install media or the root of your tools installation.
@REM THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
@REM

@ECHO OFF

@REM This file sets BSP-specific environment variables that are active during
@REM build.  It defines the default state.  The environment variable below,
@REM for example, will affect platform.reg.

@REM This variable excludes the sample registry entry in platform.reg from
@REM being included in the initial device registry.

set BSP_DISPLAY_HFLAT=1
