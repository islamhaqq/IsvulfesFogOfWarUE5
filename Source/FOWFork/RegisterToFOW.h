// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Components/ActorComponent.h"
#include "FogOfWarManager.h"
#include "RegisterToFOW.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class FOWFORK_API URegisterToFOW : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URegisterToFOW();

	// Called when the game starts
	virtual void BeginPlay() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/*Select the FOW Manager*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = FogOfWar)
		AFogOfWarManager* Manager = nullptr;
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
