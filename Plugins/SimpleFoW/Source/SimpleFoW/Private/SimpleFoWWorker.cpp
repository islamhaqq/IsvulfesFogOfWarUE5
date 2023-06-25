// Fill out your copyright notice in the Description page of Project Settings.

#include "SimpleFoWWorker.h"

#include "MathUtil.h"
#include "SimpleFoWManager.h"
#include "RegisterToSimpleFoW.h"
#include "HAL/RunnableThread.h"

FSimpleFoWWorker::FSimpleFoWWorker(ASimpleFoWManager* manager)
{
	Manager = manager;
	Thread = FRunnableThread::Create(this, TEXT("ASimpleFoWWorker"), 0U, TPri_BelowNormal);
}

FSimpleFoWWorker::~FSimpleFoWWorker()
{
	delete Thread;
	Thread = nullptr;
}

void FSimpleFoWWorker::ShutDown()
{
	Stop();
	Thread->WaitForCompletion();
}

bool FSimpleFoWWorker::Init()
{
	if (Manager)
	{
		Manager->GetWorld()->GetFirstPlayerController()->ClientMessage("Fog of War worker thread started");
		return true;
	}
	
	return false;
}

uint32 FSimpleFoWWorker::Run()
{
	FPlatformProcess::Sleep(0.03f);
	while (StopTaskCounter.GetValue() == 0)
	{
		float Time = 0.0f;
		if (Manager && Manager->GetWorld())
		{
			Time = Manager->GetWorld()->TimeSeconds;
		}
		
		if (!Manager->bHasFOWTextureUpdate)
		{
			UpdateFowTexture();
			if (Manager && Manager->GetWorld())
			{
				Manager->fowUpdateTime = Manager->GetWorld()->TimeSince(Time);
			}
		}
		
		FPlatformProcess::Sleep(0.1f);
	}
	
	return 0;
}

void FSimpleFoWWorker::UpdateFowTexture()
{
	Manager->LastFrameTextureData = TArray<FColor>(Manager->TextureData);
	const uint32 HalfTextureSize = Manager->TextureSize / 2;
	const int32 SignedSize = static_cast<int32>(Manager->TextureSize);
	TSet<FVector2D> CurrentlyInSight;
	TSet<FVector2D> TexelsToBlur;
	FVector2D SightTexels;
	const float Dividend = 100.0f / Manager->SamplesPerMeter;
	for (auto Itr(Manager->FowActors.CreateIterator()); Itr; ++Itr)
	{
		// if you experience an occasional crash
		if (StopTaskCounter.GetValue() != 0)
		{
			return;
		}
		
		//Find actor position
		if (!*Itr || !(*Itr)->FindComponentByClass<URegisterToSimpleFoW>())
		{
			return;
		}
		
		const FVector Position = (*Itr)->GetActorLocation();

		//Get sight range from FOWComponent
		const URegisterToSimpleFoW* FOWComponent = (*Itr)->FindComponentByClass<URegisterToSimpleFoW>();
		if (IsValid(FOWComponent))
		{
			SightTexels = FOWComponent->SightRange * Manager->SamplesPerMeter;
		}

		//We divide by 100.0 because 1 texel equals 1 meter of visibility-data.
		int32 PosX = static_cast<int32>(Position.X / Dividend) + HalfTextureSize;
		int32 PosY = static_cast<int32>(Position.Y / Dividend) + HalfTextureSize;
		
		//FVector2D fractions = FVector2D(modf(position.X / 50.0f, &integerX), modf(position.Y / 50.0f, &integerY));
		FVector2D TextureSpacePos = FVector2D(PosX, PosY);
		int32 Size = static_cast<int32>(Manager->TextureSize);

		FCollisionQueryParams queryParams(FName(TEXT("FOW trace")), false, (*Itr));
		int32 halfKernelSize = (Manager->blurKernelSize - 1) / 2;

		//Store the positions we want to blur
		for (int32 y = PosY - SightTexels.Y - halfKernelSize; y <= PosY + SightTexels.Y + halfKernelSize; y++) {
			for (int32 x = PosX - SightTexels.X - halfKernelSize; x <= PosX + SightTexels.X + halfKernelSize; x++) {
				if (x > 0 && x < Size && y > 0 && y < Size) {
					TexelsToBlur.Add(FIntPoint(x, y));
				}
			}
		}

		//This is checking if the current actor is able to:
		//A. Fully unveil the texels, B. unveil FOW, C, Unveil Terra Incognita
		//Accessing the registerToFOW property Unfog boolean
		//I declared the .h file for RegisterToFOW
		//Dont forget the braces >()

		if (*Itr != nullptr)
		{
			bIsWriteUnFog = (*Itr)->FindComponentByClass<URegisterToSimpleFoW>()->WriteUnFog;
			bIsWriteFow = (*Itr)->FindComponentByClass<URegisterToSimpleFoW>()->WriteFow;
			bIsWriteTerraIncog = (*Itr)->FindComponentByClass<URegisterToSimpleFoW>()->WriteTerraIncog;
			bUseLineOfSight = (*Itr)->FindComponentByClass<URegisterToSimpleFoW>()->bUseLineOfSight;
		}

		if (bIsWriteUnFog)
		{
			//Unveil the positions our actors are currently looking at
			for (int y = PosY - SightTexels.Y; y <= PosY + SightTexels.Y; y++)
			{
				for (int x = PosX - SightTexels.X; x <= PosX + SightTexels.X; x++)
				{
					//Kernel for radial sight
					if (x > 0 && x < Size && y > 0 && y < Size)
					{
						FVector2D currentTextureSpacePos = FVector2D(x, y);
						int length = (int)(TextureSpacePos - currentTextureSpacePos).Size();
						if (length <= SightTexels.Size() / 2) {
							FVector currentWorldSpacePos = FVector(
								((x - (int)HalfTextureSize)) * Dividend,
								((y - (int)HalfTextureSize)) * Dividend,
								Position.Z);

							//CONSIDER: This is NOT the most efficient way to do conditional unfogging. With long view distances and/or a lot of actors affecting the FOW-data
							//it would be preferrable to not trace against all the boundary points and internal texels/positions of the circle, but create and cache "rasterizations" of
							//viewing circles (using Bresenham's midpoint circle algorithm) for the needed sightranges, shift the circles to the actor's location
							//and just trace against the boundaries.
							//We would then use Manager->GetWorld()->LineTraceSingle() and find the first collision texel. Having found the nearest collision
							//for every ray we would unveil all the points between the collision and origo using Bresenham's Line-drawing algorithm.
							//However, the tracing doesn't seem like it takes much time at all (~0.02ms with four actors tracing circles of 18 texels each),
							//it's the blurring that chews CPU..

							if (!bUseLineOfSight || !Manager->GetWorld()->LineTraceTestByChannel(Position, currentWorldSpacePos, ECC_WorldStatic, queryParams)) {

								//Is the actor able to affect the terra incognita

								if (bIsWriteTerraIncog) {
									//if the actor is able then
									//Unveil the positions we are currently seeing
									Manager->UnfoggedData[x + y * Manager->TextureSize] = true;
								}
								//Store the positions we are currently seeing.
								CurrentlyInSight.Add(FVector2D(x, y));

							}
						}
					}
				}
			}
		}

		//Is the current actor marked for checking if is in terra incognita

		if (*Itr != nullptr)
		{
			bCheckActorInTerraIncog = (*Itr)->FindComponentByClass<URegisterToSimpleFoW>()->bCheckActorTerraIncog;
		}
		
		if (bCheckActorInTerraIncog)
		{
			//if the current position textureSpacePosXY in the UnfoggedData bool array is false the actor is in the Terra Incognita
			if (Manager->UnfoggedData[TextureSpacePos.X + TextureSpacePos.Y * Manager->TextureSize] == false)
			{
				(*Itr)->FindComponentByClass<URegisterToSimpleFoW>()->isActorInTerraIncog = true;
			}
			else
			{
				(*Itr)->FindComponentByClass<URegisterToSimpleFoW>()->isActorInTerraIncog = false;
			}
		}
	}

	if (Manager->GetIsBlurEnabled())
	{
		//Horizontal blur pass
		int Offset = FMathf::Floor(Manager->blurKernelSize / 2.0f);
		for (auto Itr(TexelsToBlur.CreateIterator()); Itr; ++Itr)
		{
			int x = (Itr)->IntPoint().X;
			int y = (Itr)->IntPoint().Y;
			float Sum = 0;
			for (int i = 0; i < Manager->blurKernelSize; i++) {
				int shiftedIndex = i - Offset;
				if (x + shiftedIndex >= 0 && x + shiftedIndex <= SignedSize - 1)
				{
					if (Manager->UnfoggedData[x + shiftedIndex + (y * SignedSize)])
					{
						//If we are currently looking at a position, unveil it completely
						if (CurrentlyInSight.Contains(FVector2D(x + shiftedIndex, y)))
						{
							Sum += (Manager->blurKernel[i] * 255);
						}
						//If this is a previously discovered position that we're not currently looking at, put it into a "shroud of darkness".
						else
						{
							//sum += (Manager->blurKernel[i] * 100);
							Sum += (Manager->blurKernel[i] * Manager->FowMaskColor); //i mod this to make the blurred color unveiled
						}
					}
				}
			}
			
			Manager->HorizontalBlurData[x + y * SignedSize] = static_cast<uint8>(Sum);
		}


		//Vertical blur pass
		for (auto Itr(TexelsToBlur.CreateIterator()); Itr; ++Itr)
		{
			int x = (Itr)->IntPoint().X;
			int y = (Itr)->IntPoint().Y;
			float Sum = 0;
			for (int i = 0; i < Manager->blurKernelSize; i++)
			{
				int ShiftedIndex = i - Offset;
				if (y + ShiftedIndex >= 0 && y + ShiftedIndex <= SignedSize - 1)
				{
					Sum += (Manager->blurKernel[i] * Manager->HorizontalBlurData[x + (y + ShiftedIndex) * SignedSize]);
				}
			}
			
			Manager->TextureData[x + y * SignedSize] = FColor(static_cast<uint8>(Sum), static_cast<uint8>(Sum), static_cast<uint8>(Sum), 255);
		}
	}
	else
	{
		for (int y = 0; y < SignedSize; y++)
		{
			for (int x = 0; x < SignedSize; x++)
			{
				if (Manager->UnfoggedData[x + (y * SignedSize)])
				{
					//If we are currently looking at a position, unveil it completely
					//if the vectors are inside de TSet
					if (CurrentlyInSight.Contains(FVector2D(x, y)))
					{
						Manager->TextureData[x + y * SignedSize] = FColor(Manager->UnfogColor, Manager->UnfogColor, Manager->UnfogColor, 255);

						if (Manager->bIsFowTimerEnabled)
						{
							Manager->FOWArray[x + (y * SignedSize)] = false;
						}
					}
					//If this is a previously discovered position that we're not currently looking at, put it into a "shroud of darkness".
					else
					{
						Manager->TextureData[x + y * SignedSize] = FColor(Manager->FowMaskColor, Manager->FowMaskColor, Manager->FowMaskColor, 255);
						//This line sets the color to black again in the textureData, sets the UnfoggedData to False

						if (Manager->bIsFowTimerEnabled)
						{
							Manager->FOWArray[x + (y * SignedSize)] = true;

							if (Manager->FOWTimeArray[x + y * SignedSize] >= Manager->FowTimeLimit)
							{
								//setting the color
								Manager->TextureData[x + y * SignedSize] = FColor(0.0, 0.0, 0.0, 255.0);
								//from FOW to TerraIncognita
								Manager->UnfoggedData[x + (y * SignedSize)] = false;
								//reset the value
								Manager->FOWArray[x + (y * SignedSize)] = false;
							}
						}
					}
				}


			}
		}
	}

	Manager->bHasFOWTextureUpdate = true;
}


void FSimpleFoWWorker::Stop()
{
	StopTaskCounter.Increment();
}


