// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "HAL/Runnable.h"

/**
* Worker thread for updating the fog of war data.
*/
class ASimpleFoWManager;

class SIMPLEFOW_API FSimpleFoWWorker : public FRunnable
{
	//Thread to run the FRunnable on
	FRunnableThread* Thread = nullptr;

	//Pointer to our manager
	ASimpleFoWManager* Manager = nullptr;

	//Thread safe counter
	FThreadSafeCounter StopTaskCounter;

public:
	FSimpleFoWWorker() = default;
	explicit FSimpleFoWWorker(ASimpleFoWManager* manager);
	virtual ~FSimpleFoWWorker() override;

	//FRunnable interface
	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Stop() override;

	//Method to perform work
	void UpdateFowTexture();
	bool bShouldUpdate = false;
	bool bIsWriteUnFog = false;
	bool bIsWriteFow = false;
	bool bIsWriteTerraIncog = false;
	bool bCheckActorInTerraIncog = false;//Bool, is the actor in terra incognita territory
	bool bUseLineOfSight = true;

	void ShutDown();
};