// Fill out your copyright notice in the Description page of Project Settings.

#include "RegisterToSimpleFoW.h"
#include "SimpleFoWManager.h"

// Sets default values for this component's properties
URegisterToSimpleFoW::URegisterToSimpleFoW()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}


// Called when the game starts
void URegisterToSimpleFoW::BeginPlay()
{
	Super::BeginPlay();
	FString ObjectName = GetOwner()->GetName();
	UE_LOG(LogTemp, Log, TEXT("I am alive %s"), *ObjectName);

	//registering the actor to the FOW Manager

	if (Manager != nullptr) {
		UE_LOG(LogTemp, Log, TEXT("Found Manager"));

		Manager->RegisterFowActor(GetOwner());
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("Please attach a Simple FOW Manager"));
	}

}
