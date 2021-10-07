#include<tchar.h>
#include<stdio.h>
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>
#include"MyAudioSink.h"
#include "Դ.h"

//-----------------------------------------------------------
// Record an audio stream from the default audio capture
// device. The RecordAudioStream function allocates a shared
// buffer big enough to hold one second of PCM audio data.
// The function uses this buffer to stream data from the
// capture device. The main loop runs every 1/2 second.
//-----------------------------------------------------------

// REFERENCE_TIME time units per second and per millisecond
#define REFTIMES_PER_SEC  10000000
#define REFTIMES_PER_MILLISEC  10000

#define EXIT_ON_ERROR(hres,msg)  \
              if (FAILED(hres)) { printf(msg);goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient = __uuidof(IAudioClient);
const IID IID_IAudioCaptureClient = __uuidof(IAudioCaptureClient);

HRESULT RecordAudioStream(MyAudioSink* pMySink)
{
    HRESULT hr;
    REFERENCE_TIME hnsRequestedDuration = REFTIMES_PER_SEC;
    REFERENCE_TIME hnsActualDuration;
    UINT32 bufferFrameCount;
    UINT32 numFramesAvailable;
    IMMDeviceEnumerator* pEnumerator = NULL;
    IMMDevice* pDevice = NULL;
    IAudioClient* pAudioClient = NULL;
    IAudioCaptureClient* pCaptureClient = NULL;
    WAVEFORMATEX* pwfx = NULL;
    UINT32 packetLength = 0;
    BOOL bDone = FALSE;
    BYTE* pData;
    DWORD flags;

    hr = CoCreateInstance(
        CLSID_MMDeviceEnumerator, NULL,
        CLSCTX_ALL, IID_IMMDeviceEnumerator,
        (void**)&pEnumerator);
    EXIT_ON_ERROR(hr,"CoCreateInstance error\n")

        hr = pEnumerator->GetDefaultAudioEndpoint(
            eCapture, eConsole, &pDevice);
    EXIT_ON_ERROR(hr,"GetDefaultAudioEndpoint Error\n")

        hr = pDevice->Activate(
            IID_IAudioClient, CLSCTX_ALL,
            NULL, (void**)&pAudioClient);
    EXIT_ON_ERROR(hr,"Active Error\n")

        hr = pAudioClient->GetMixFormat(&pwfx);
    EXIT_ON_ERROR(hr,"GetMixFormat Error\n")

        hr = pAudioClient->Initialize(
            AUDCLNT_SHAREMODE_SHARED,
            0,
            hnsRequestedDuration,
            0,
            pwfx,
            NULL);
    EXIT_ON_ERROR(hr, "client initialize Error\n")

        // Get the size of the allocated buffer.
        hr = pAudioClient->GetBufferSize(&bufferFrameCount);
    EXIT_ON_ERROR(hr, "GetBufferSize Error\n")

        hr = pAudioClient->GetService(
            IID_IAudioCaptureClient,
            (void**)&pCaptureClient);
    EXIT_ON_ERROR(hr, "GetService Error\n")

        // Notify the audio sink which format to use.
        hr = pMySink->SetFormat(pwfx);
    EXIT_ON_ERROR(hr,"Sink SetFormat Error\n")

        // Calculate the actual duration of the allocated buffer.
        hnsActualDuration = (double)REFTIMES_PER_SEC *
        bufferFrameCount / pwfx->nSamplesPerSec;

    hr = pAudioClient->Start();  // Start recording.
    EXIT_ON_ERROR(hr, "Client Start Error\n")

        // Each loop fills about half of the shared buffer.
        while (bDone == FALSE)
        {
            // Sleep for half the buffer duration.
            Sleep(hnsActualDuration / REFTIMES_PER_MILLISEC / 2);

            hr = pCaptureClient->GetNextPacketSize(&packetLength);
            EXIT_ON_ERROR(hr, "Clinet GetNextPacketSize Error\n")

                while (packetLength != 0)
                {
                    // Get the available data in the shared buffer.
                    hr = pCaptureClient->GetBuffer(
                        &pData,
                        &numFramesAvailable,
                        &flags, NULL, NULL);
                    EXIT_ON_ERROR(hr, "Clinet GetBuffer Error\n")

                        if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
                        {
                            pData = NULL;  // Tell CopyData to write silence.
                        }

                    // Copy the available capture data to the audio sink.
                    hr = pMySink->CopyData(
                        pData, numFramesAvailable, &bDone);
                    EXIT_ON_ERROR(hr, "Sink CopyData Error\n")

                        hr = pCaptureClient->ReleaseBuffer(numFramesAvailable);
                    EXIT_ON_ERROR(hr, "Client ReleaseBuffer Error\n")

                        hr = pCaptureClient->GetNextPacketSize(&packetLength);
                    EXIT_ON_ERROR(hr, "Clinet GetNextPacketSize Error\n")
                }
        }

    hr = pAudioClient->Stop();  // Stop recording.
    EXIT_ON_ERROR(hr, "Clinet Stop Error\n")

        Exit:
    CoTaskMemFree(pwfx);
    SAFE_RELEASE(pEnumerator)
        SAFE_RELEASE(pDevice)
        SAFE_RELEASE(pAudioClient)
        SAFE_RELEASE(pCaptureClient)

        return hr;
}