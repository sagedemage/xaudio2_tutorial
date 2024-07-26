/* Third Party Libraries*/
#include <comdef.h>

/* Standard Libraries */
#include <iostream>
#include <string>

/* Local header files */
#include "xaudio_driver.h"
#include "hresult_debugger.h"

HRESULT FindChunk(HANDLE hFile, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition) {
	HRESULT hr = S_OK;

	DWORD fileP = SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	if (fileP == INVALID_SET_FILE_POINTER) {
		hr = HRESULT_FROM_WIN32(GetLastError());

		verbose_debug_hresult(hr, "SetFilePointer Error: INVALID_SET_FILE_POINTER in FindChunk");

		return hr;
	}

	DWORD dwChunkType;
	DWORD dwChunkDataSize;
	DWORD dwRIFFDataSize = 0;
	DWORD dwFileType;
	DWORD bytesRead = 0;
	DWORD dwOffset = 0;

	while (hr == S_OK) {
		DWORD dwRead;
		if (ReadFile(hFile, &dwChunkType, sizeof(DWORD), &dwRead, NULL) == 0) {
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
		if (ReadFile(hFile, &dwChunkDataSize, sizeof(DWORD), &dwRead, NULL) == 0) {
			hr = HRESULT_FROM_WIN32(GetLastError());
		}

		switch (dwChunkType) {
		case fourccRIFF:
			dwRIFFDataSize = dwChunkDataSize;
			dwChunkDataSize = 4;
			if (ReadFile(hFile, &dwFileType, sizeof(DWORD), &dwRead, NULL) == 0) {
				hr = HRESULT_FROM_WIN32(GetLastError());
			}
			break;
		default:
			// the dwChunkDataSize is not the right parameter
			DWORD fileP = SetFilePointer(hFile, dwChunkDataSize, NULL, FILE_CURRENT);
			if (fileP == INVALID_SET_FILE_POINTER) {
				return HRESULT_FROM_WIN32(GetLastError());
			}
		}

		dwOffset += sizeof(DWORD) * 2;

		if (dwChunkType == fourcc) {
			dwChunkSize = dwChunkDataSize;
			dwChunkDataPosition = dwOffset;
			return S_OK;
		}

		dwOffset += dwChunkDataSize;

		if (bytesRead >= dwRIFFDataSize) {
			return S_FALSE;
		}
	}

	return S_OK;
}

HRESULT ReadChunkData(HANDLE hFile, LPVOID buffer, DWORD buffersize, DWORD bufferoffset) {
	HRESULT hr = S_OK;

	DWORD fileP = SetFilePointer(hFile, bufferoffset, NULL, FILE_BEGIN);

	if (fileP == INVALID_SET_FILE_POINTER) {
		HRESULT hr = HRESULT_FROM_WIN32(GetLastError());

		verbose_debug_hresult(hr, "SetFilePointer Error: INVALID_SET_FILE_POINTER in ReadChunkData");

		return hr;
	}

	DWORD dwRead;
	if (ReadFile(hFile, buffer, buffersize, &dwRead, NULL) == 0) {
		hr = HRESULT_FROM_WIN32(GetLastError());
	}
	return hr;
}

bool LoadWaveAudioFile(LPCSTR audioFilePath, WAVEFORMATEXTENSIBLE* wfx, XAUDIO2_BUFFER* buffer) {
	HRESULT hr;
	HANDLE hFile = CreateFileA(audioFilePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);

	if (hFile == INVALID_HANDLE_VALUE) {
		MessageBox(0, L"Failed CreateFileA in LoadWaveAudioFile", 0, 0);

		hr = HRESULT_FROM_WIN32(GetLastError());

		verbose_debug_hresult(hr, "Failed CreateFileA in LoadWaveAudioFile");

		return false;
	}

	DWORD fileP = SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

	if (fileP == INVALID_SET_FILE_POINTER) {
		MessageBox(0, L"Failed SetFilePointer in LoadWaveAudioFile", 0, 0);

		hr = HRESULT_FROM_WIN32(GetLastError());
		verbose_debug_hresult(hr, "SetFilePointerEx Error: INVALID_SET_FILE_POINTER in LoadWaveAudioFile");

		return false;
	}

	DWORD dwChunkSize;
	DWORD dwChunkPosition;

	/* RIFF chunk */
	FindChunk(hFile, fourccRIFF, dwChunkSize, dwChunkPosition);
	DWORD filetype;
	ReadChunkData(hFile, &filetype, sizeof(DWORD), dwChunkPosition);

	if (filetype != fourccWAVE) {
		MessageBox(0, L"Filetype not WAVE", 0, 0);

		std::string debug_msg = "Filetype not WAVE\n";
		OutputDebugStringA(debug_msg.c_str());

		return false;
	}

	/* fmt sub-chunk */
	FindChunk(hFile, fourccFMT, dwChunkSize, dwChunkPosition);

	// Locate the 'fmt' chunk, and copy its contents into a WAVEFORMATEXTENSIBLE structure
	ReadChunkData(hFile, &wfx, dwChunkSize, dwChunkPosition);

	/* data sub-chunk */
	FindChunk(hFile, fourccDATA, dwChunkSize, dwChunkPosition);
	BYTE* pDataBuffer = new BYTE[dwChunkSize];
	ReadChunkData(hFile, pDataBuffer, dwChunkSize, dwChunkPosition);

	float playLengthMultiplier = 1.0f;

	UINT32 PlayLength = UINT32(dwChunkSize * playLengthMultiplier);

	// Setup Buffer attributes
	buffer->Flags = XAUDIO2_END_OF_STREAM;
	buffer->AudioBytes = dwChunkSize;
	buffer->pAudioData = pDataBuffer;

	buffer->PlayBegin = 0; // Optional
	buffer->PlayLength = PlayLength; // Optional
	buffer->LoopBegin = buffer->PlayBegin + buffer->PlayLength - 1; // Optional
	buffer->LoopLength = 0; // Optional
	buffer->LoopCount = 1;
	buffer->pContext = NULL; // Optional

	return true;
}

bool PlayAudioSound(IXAudio2* pXAudio2, WAVEFORMATEXTENSIBLE wfx, XAUDIO2_BUFFER buffer) {
	HRESULT hr;
	IXAudio2SourceVoice* pSourceVoice;

	/* WAVEFORMATEX Format of the WAVE */
	/*
	  8.000 kHz =  8000L
	 11.025 kHz = 11025L
	 22.050 kHz = 22050L
	 44.100 kHz = 44100L
	 */
	const long nSamplesPerSec = 11025L;
	const int nChannels = 1;
	const int wBitsPerSample = 8;
	const int nBlockAlign = (nChannels * wBitsPerSample) / 8;

	wfx.Format.wFormatTag = WAVE_FORMAT_PCM;
	wfx.Format.nChannels = nChannels;
	wfx.Format.nSamplesPerSec = nSamplesPerSec;
	wfx.Format.nAvgBytesPerSec = nSamplesPerSec * nBlockAlign;
	wfx.Format.nBlockAlign = nBlockAlign;
	wfx.Format.wBitsPerSample = wBitsPerSample;
	wfx.Format.cbSize = 22; // Optional

	/* Sample Format */
	wfx.Samples.wValidBitsPerSample = wBitsPerSample;
	wfx.Samples.wSamplesPerBlock = 0;
	wfx.Samples.wReserved = 0;

	// dwChannelMask Specifies which channels are present in the multichannel stream
	wfx.dwChannelMask = SPEAKER_BACK_CENTER;

	// Subformat
	wfx.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;

	// Initialize XAudio source voice
	hr = pXAudio2->CreateSourceVoice(&pSourceVoice, (WAVEFORMATEX*)&wfx, 0, XAUDIO2_DEFAULT_FREQ_RATIO, NULL, NULL, NULL);

	if (FAILED(hr)) {
		MessageBox(0, L"Failed CreateSourceVoice in PlayAudioSound", 0, 0);

		verbose_debug_hresult(hr, "CreateSourceVoice Error in PlayAudioSound");

		return false;
	}

	// Adds a new audio buffer to the voice queue
	hr = pSourceVoice->SubmitSourceBuffer(&buffer);

	if (FAILED(hr)) {
		MessageBox(0, L"Failed SubmitSourceBuffer in PlayAudioSound", 0, 0);

		verbose_debug_hresult(hr, "SubmitSourceBuffer Error in PlayAudioSound");

		return false;
	}

	// Set frequency of the source voice
	float sourceRate = 1;
	float targetRate = 32;
	float frequencyRatio = sourceRate / targetRate;
	pSourceVoice->SetFrequencyRatio(frequencyRatio);

	// Start the XAudio source voice
	hr = pSourceVoice->Start(0);

	if (FAILED(hr)) {
		MessageBox(0, L"Failed Start Source Voice", 0, 0);

		verbose_debug_hresult(hr, "Failed Start Source Voice in PlayAudioSound");

		return false;
	}

	return true;
}