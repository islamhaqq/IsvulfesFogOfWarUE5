// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "SimpleFoWManager.h"
#include "RegisterToSimpleFoW.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SIMPLEFOW_API URegisterToSimpleFoW : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URegisterToSimpleFoW();

	// Called when the game starts
	virtual void BeginPlay() override;
	
	/*Select the FOW Manager*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	ASimpleFoWManager* Manager = nullptr;
	
	/*Is the actor able to influence unfogged texels*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	bool WriteUnFog = true;
	
	/*Is the actor able to influence fog of war texels*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	bool WriteFow = true;
	
	/*Is the actor able to influence terra incognita texels*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	bool WriteTerraIncog = true;
	
	/*Check if the actor is in terra incognita*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	bool bCheckActorTerraIncog = false;
	
	/*Should the actor reveal texels that are out of LOS*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	bool bUseLineOfSight = true;
	
	/**/
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = FogOfWar)
	bool isActorInTerraIncog = false;
	
	/*How far will the actor be able to see*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
	FVector2D SightRange = FVector2D(9.0f);
};
