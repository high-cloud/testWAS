#pragma once


#include<tchar.h>
#include<stdio.h>
#include <MMDeviceAPI.h>
#include <AudioClient.h>
#include <AudioPolicy.h>

class MyAudioSink
{
public:
	HRESULT SetFormat(WAVEFORMATEX*);
	HRESULT CopyData(BYTE*, UINT32, BOOL*);
};

