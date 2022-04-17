//
// Copyright (c) Microsoft Corporation.  All rights reserved.
//
//
// Use of this sample source code is subject to the terms of the Microsoft
// license agreement under which you licensed this sample source code. If
// you did not accept the terms of the license agreement, you are not
// authorized to use this sample source code. For the terms of the license,
// please see the license agreement between you and Microsoft or, if applicable,
// see the LICENSE.RTF on your install media or the root of your tools installation.
// THE SAMPLE SOURCE CODE IS PROVIDED "AS IS", WITH NO WARRANTIES OR INDEMNITIES.
//

//
//  Module Name:
//
//      wavepdd.c
//
//  Abstract:
//      The PDD (Platform Dependent Driver) is responsible for
//      communicating with the audio circuit to start and stop playback
//      and/or recording and initialize and deinitialize the circuits.
//
//  Functions:
//      PDD_AudioGetInterruptType
//      PDD_AudioMessage
//      PDD_AudioInitialize
//      PDD_AudioDeinitialize
//      PDD_AudioPowerHandler
//      PDD_WaveProc
//
//  Notes:
//
// -----------------------------------------------------------------------------
#include <windows.h>
#include <types.h>
#include <memory.h>
#include <nkintr.h>
#include <wavedbg.h>
#include <waveddsi.h>

#define mmio_write32(reg, data)  *(volatile DWORD*)(reg) = (data)
#define mmio_read32(reg)  *(volatile DWORD*)(reg)

DWORD gIntrAudio;

struct device_state
{
	DWORD io_base;
	DWORD fifo_size;
	BOOL in_use;
	DWORD set_rate;

	DWORD play_state;
	DWORD volume;
} state;

enum pl041_regs
{
	AACI_TXCR1 = 0x04,
	AACI_SR1 = 0x08,
	AACI_ISR1 = 0x0C,
	AACI_IE1 = 0x10,

	AACI_SL1TX = 0x54,
	AACI_SL2TX = 0x5C,

	AACI_INTCLR = 0x74,
	AACI_MAINCR = 0x78,

	AACI_DR1 = 0x90,

	AACI_PERIPHID3 = 0xFEC,

	LM_DAC_RATE = 0x2C,
};

MMRESULT private_WaveGetDevCaps(WAPI_INOUT apidir, PVOID pCaps, UINT wSize)
{
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveGetDevCaps\r\n")));

    PWAVEOUTCAPS pwoc = (PWAVEOUTCAPS)pCaps;

    if (pCaps == NULL)
        return MMSYSERR_NOERROR;

    pwoc->wMid = MM_MICROSOFT;
    pwoc->wPid = 24;
    pwoc->vDriverVersion = 0x0001;

    StringCbPrintf(pwoc->szPname, sizeof(pwoc->szPname), TEXT("PL041 Audio"));

    pwoc->dwFormats = WAVE_FORMAT_1S16 | WAVE_FORMAT_2S16 | WAVE_FORMAT_4S16; // stereo, 16 bit

    pwoc->wChannels = 2;
    pwoc->dwSupport = 0;

    return MMSYSERR_NOERROR;
}

VOID private_WaveStart (WAPI_INOUT apidir, PWAVEHDR pwh)
{
    DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveStart\r\n")));

	state.play_state = 1;
	mmio_write32(state.io_base + AACI_IE1, (1 << 2)); // fifo half
}

VOID private_WaveContinue(WAPI_INOUT apidir, PWAVEHDR pwh)
{
    DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveContinue %x\r\n"), (DWORD)pwh));

	DWORD buf_samples = pwh->dwBufferLength / 4;
	DWORD buf_pos = pwh->dwBytesRecorded / 4;

	DWORD max_transfer = buf_samples;
	if (max_transfer - buf_pos > state.fifo_size / 2)
		max_transfer = buf_pos + state.fifo_size / 2;

	int vol = state.volume >> 16;
	DWORD *samples = (DWORD*)pwh->lpData;
	for (DWORD i = buf_pos; i < max_transfer; i++) {
		short chan1 = samples[i] & 0xFFFF;
		short chan2 = samples[i] >> 16;
		chan1 = chan1 * vol / 0x10000;
		chan2 = chan2 * vol / 0x10000;
		DWORD sample = (DWORD)chan1 | ((DWORD)chan2 << 16);
		mmio_write32(state.io_base + AACI_DR1, sample);
	}

	pwh->dwBytesRecorded = max_transfer * 4;

	mmio_write32(state.io_base + AACI_IE1, (1 << 2)); // fifo half
}

VOID private_WaveStop(WAPI_INOUT apidir)
{
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveStop\r\n")));

	state.play_state = 0;
	mmio_write32(state.io_base + AACI_IE1, 0);
}

VOID private_WaveStandby(WAPI_INOUT apidir)
{
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveStandby\r\n")));

	state.play_state = 0;
}

VOID private_WaveEndOfData(WAPI_INOUT apidir)
{
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveEndOfData\r\n")));

	if (!state.play_state) {
		DEBUGMSG(ZONE_PDD, (TEXT("pl041: already stopped\r\n")));
		return;
	}

	state.play_state = 2;
	mmio_write32(state.io_base + AACI_IE1, (1 << 0)); // fifo empty
}

MMRESULT private_WaveOpen(WAPI_INOUT apidir, LPWAVEFORMATEX lpFormat, BOOL fQueryFormatOnly)
{
    MMRESULT mmRet = MMSYSERR_NOERROR;

	DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveOpen, rate=%u\r\n"), lpFormat->nSamplesPerSec));

    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM ||
		lpFormat->nChannels  != 2 ||
		lpFormat->wBitsPerSample != 16)
    {
        return WAVERR_BADFORMAT;
    }

    if (fQueryFormatOnly)
		return MMSYSERR_NOERROR;

	if (state.in_use) {
		DEBUGMSG(ZONE_WARN, (TEXT("pl041: already in use\r\n")));
		return MMSYSERR_ALLOCATED;
	}

	state.in_use = TRUE;
	if (state.set_rate != lpFormat->nSamplesPerSec) {
		state.set_rate = lpFormat->nSamplesPerSec;
		mmio_write32(state.io_base + AACI_SL2TX, state.set_rate << 4);
		mmio_write32(state.io_base + AACI_SL1TX, LM_DAC_RATE << 12);
	}

    return MMSYSERR_NOERROR;
}

VOID private_WaveClose()
{
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: WaveClose\r\n")));

	if (state.play_state) {
		DEBUGMSG(ZONE_WARN, (TEXT("pl041: still playing during WaveClose\r\n")));
		private_WaveStop(WAPI_OUT);
	}
	state.in_use = FALSE;
}

// -----------------------------
// DDSI functions
// -----------------------------

AUDIO_STATE PDD_AudioGetInterruptType(VOID)
{
	mmio_write32(state.io_base + AACI_IE1, 0);

	if (state.play_state == 1) {
		return AUDIO_STATE_OUT_PLAYING;
	}
	if (state.play_state == 2) {
		return AUDIO_STATE_OUT_STOPPED;
	}

	DEBUGMSG(ZONE_PDD, (TEXT("pl041: unexpected interrupt\r\n")));

    return AUDIO_STATE_IGNORE;
}

BOOL RegGetValue(const WCHAR* ActiveKey, const WCHAR* pwsName, PDWORD pdwValue);

BOOL PDD_AudioInitialize(DWORD dwIndex)
{
	//dpCurSettings.ulZoneMask = (1 << 14) | (1 << 15) | (1 << 6);
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: AudioInit\r\n")));

	if (!RegGetValue((LPWSTR)dwIndex, TEXT("IoBase"), &state.io_base)) {
		DEBUGMSG(ZONE_ERROR, (TEXT("pl041: failed to get IoBase\r\n")));
		return FALSE;
	}

	if (!RegGetValue((LPWSTR)dwIndex, TEXT("SysIntr"), &gIntrAudio)) {
		DEBUGMSG(ZONE_ERROR, (TEXT("pl041: failed to get SysIntr\r\n")));
		return FALSE;
	}

	state.volume = 0xFFFFFFFF;

	mmio_write32(state.io_base + AACI_MAINCR, (1 << 0));
	mmio_write32(state.io_base + AACI_TXCR1, (1 << 16) | (1 << 15) | (1 << 3) | (1 << 0)); // TxFen, TxCompact, SLOT3, TxEn

	DWORD info = mmio_read32(state.io_base + AACI_PERIPHID3);
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: PERIPHID3: %x\r\n"), info));
	switch (info >> 3) {
		case 0: state.fifo_size = 4; break;
		case 1: state.fifo_size = 16; break;
		case 2: state.fifo_size = 32; break;
		case 3: state.fifo_size = 64; break;
		case 4: state.fifo_size = 128; break;
		case 5: state.fifo_size = 256; break;
		case 6: state.fifo_size = 512; break;
		case 7: state.fifo_size = 1024; break;
	}

    return TRUE;
}

VOID PDD_AudioDeinitialize(VOID)
{
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: AudioDeinit\r\n")));

	mmio_write32(state.io_base + AACI_MAINCR, 0);
	state.in_use = FALSE;
	state.set_rate = 0;
	state.play_state = 0;
}

VOID PDD_AudioPowerHandler(BOOL)
{
	DEBUGMSG(ZONE_PDD, (TEXT("pl041: AudioPowerHandler\r\n")));
}

DWORD PDD_AudioMessage(UINT, DWORD, DWORD)
{
    return MMSYSERR_NOTSUPPORTED;
}

MMRESULT PDD_WaveProc(WAPI_INOUT apidir, DWORD dwCode, DWORD dwParam1, DWORD dwParam2)
{
	if (apidir != WAPI_OUT) {
		DEBUGMSG(ZONE_WARN, (TEXT("pl041: recording not supported\r\n")));
		return MMSYSERR_NOTSUPPORTED;
	}

    switch (dwCode)
    {
    case WPDM_CLOSE:
		private_WaveClose();
        return MMSYSERR_NOERROR;

    case WPDM_CONTINUE:
        private_WaveContinue(apidir, (PWAVEHDR) dwParam1);
        return MMSYSERR_NOERROR;

    case WPDM_GETDEVCAPS:
        return private_WaveGetDevCaps(apidir, (PVOID) dwParam1, (UINT) dwParam2);

    case WPDM_OPEN:
        return private_WaveOpen(apidir, (LPWAVEFORMATEX) dwParam1, (BOOL) dwParam2);

    case WPDM_STANDBY:
		private_WaveStandby(apidir);
        return MMSYSERR_NOERROR;

	case WPDM_RESTART:
    case WPDM_START:
        private_WaveStart(apidir, (PWAVEHDR) dwParam1);
        return MMSYSERR_NOERROR;

    case WPDM_ENDOFDATA:
        private_WaveEndOfData(apidir);
        return MMSYSERR_NOERROR;

    case WPDM_PAUSE:
    case WPDM_STOP:
        private_WaveStop(apidir);
        return MMSYSERR_NOERROR;

    case WPDM_GETVOLUME:
		*((DWORD*)dwParam1) = state.volume;
		return MMSYSERR_NOERROR;

    case WPDM_SETVOLUME:
		state.volume = dwParam1;
		return MMSYSERR_NOERROR;

    case WPDM_SETMIXERVAL:
    case WPDM_GETMIXERVAL:
		DEBUGMSG(ZONE_WARN, (TEXT("pl041: unsupported WaveProc call: %u\r\n"), dwCode));
        return MMSYSERR_NOTSUPPORTED;

    default:
		break;
    }

	DEBUGMSG(ZONE_WARN, (TEXT("pl041: unknown WaveProc call: %u\r\n"), dwCode));

	return MMSYSERR_NOTSUPPORTED;
}

