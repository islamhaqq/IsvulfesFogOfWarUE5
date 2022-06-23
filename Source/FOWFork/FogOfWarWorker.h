// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Runnable.h"

/**
* Worker thread for updating the fog of war data.
*/
class AFogOfWarManager;

class FOWFORK_API AFogOfWarWorker : public FRunnable
{
	//Thread to run the FRunnable on
	FRunnableThread* Thread;

	//Pointer to our manager
	AFogOfWarManager* Manager;

	//Thread safe counter
	FThreadSafeCounter StopTaskCounter;

public:
	AFogOfWarWorker();
	AFogOfWarWorker(AFogOfWarManager* manager);
	virtual ~AFogOfWarWorker();

	//FRunnable interface
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();

	//Method to perform work
	void UpdateFowTexture();
	bool bShouldUpdate = false;
	bool isWriteUnFog = false;
	bool isWriteFow = false;
	bool isWriteTerraIncog = false;
	bool bCheckActorInTerraIncog = false;//Bool, is the actor in terra incognita territory
	bool bUseLineOfSight = true;

	void ShutDown();
};