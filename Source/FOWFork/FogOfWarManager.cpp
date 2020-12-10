// Fill out your copyright notice in the Description page of Project Settings.

#include "FogOfWarManager.h"
#include "Rendering/Texture2DResource.h"
#include "RegisterToFOW.h"

// Sets default values
AFogOfWarManager::AFogOfWarManager()
{
	PrimaryActorTick.bCanEverTick = true;
	textureRegions = new FUpdateTextureRegion2D(0, 0, 0, 0, TextureSize, TextureSize);

	//15 Gaussian samples. Sigma is 2.0.
	//CONSIDER: Calculate the kernel instead, more flexibility...
	blurKernel.Init(0.0f, blurKernelSize);
	blurKernel[0] = 0.000489f;
	blurKernel[1] = 0.002403f;
	blurKernel[2] = 0.009246f;
	blurKernel[3] = 0.02784f;
	blurKernel[4] = 0.065602f;
	blurKernel[5] = 0.120999f;
	blurKernel[6] = 0.174697f;
	blurKernel[7] = 0.197448f;
	blurKernel[8] = 0.174697f;
	blurKernel[9] = 0.120999f;
	blurKernel[10] = 0.065602f;
	blurKernel[11] = 0.02784f;
	blurKernel[12] = 0.009246f;
	blurKernel[13] = 0.002403f;
	blurKernel[14] = 0.000489f;
}

AFogOfWarManager::~AFogOfWarManager() {
	if (FowThread) {
		FowThread->ShutDown();
	}
}

void AFogOfWarManager::BeginPlay() {
	Super::BeginPlay();
	bIsDoneBlending = true;
	AFogOfWarManager::StartFOWTextureUpdate();
	//I commented this to remove the player from the FOW
	//RegisterFowActor(GetWorld()->GetFirstPlayerController()->GetPawn());

}

void AFogOfWarManager::Tick(float DeltaSeconds) {
	Super::Tick(DeltaSeconds);
	if (FOWTexture && LastFOWTexture && bHasFOWTextureUpdate && bIsDoneBlending) {
		LastFOWTexture->UpdateResource();
		UpdateTextureRegions(LastFOWTexture, (int32)0, (uint32)1, textureRegions, (uint32)(4 * TextureSize), (uint32)4, (uint8*)LastFrameTextureData.GetData(), false);
		FOWTexture->UpdateResource();
		UpdateTextureRegions(FOWTexture, (int32)0, (uint32)1, textureRegions, (uint32)(4 * TextureSize), (uint32)4, (uint8*)TextureData.GetData(), false);
		bHasFOWTextureUpdate = false;
		bIsDoneBlending = false;
		//Trigger the blueprint update
		OnFowTextureUpdated(FOWTexture, LastFOWTexture);
	}



	if (bIsFowTimerEnabled) {
		//Keeping the Measure of the time outside the Worker thread, otherwise the it does not work
		//i guest that asking for the delta seconds inside the worker that not make sense, hence, it is detached
		//from the main game thread
		//also this part of the code is NOT inside the if that checks for bIsDoneBlending
		int signedSize = (int)TextureSize;
		for (int y = 0; y < signedSize; y++) {
			for (int x = 0; x < signedSize; x++) {

				//check the FOWArray written by the worker, if is tagged for keeping the time then:
				if (FOWArray[x + y * signedSize]) {
					FOWTimeArray[x + y * signedSize] = (FOWTimeArray[x + y * signedSize]) + ( GetWorld()->GetDeltaSeconds());
				}
				//if the current value is not tagged for time, reset the value
				if (!FOWArray[x + y * signedSize]) {
					FOWTimeArray[x + y * signedSize] = 0.0f;
				}

			}
		}
	}
}

void AFogOfWarManager::StartFOWTextureUpdate() {
	if (!FOWTexture) {
		FOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
		LastFOWTexture = UTexture2D::CreateTransient(TextureSize, TextureSize);
		int arraySize = TextureSize * TextureSize;
		TextureData.Init(FColor(0, 0, 0, 255), arraySize);
		LastFrameTextureData.Init(FColor(0, 0, 0, 255), arraySize);
		HorizontalBlurData.Init(0, arraySize);
		UnfoggedData.Init(false, arraySize);
		FowThread = new AFogOfWarWorker(this);

		//Time stuff
		FOWTimeArray.Init(0.0f, arraySize);
		FOWArray.Init(false, arraySize);
	}

	//Loading texture to array
	//Copy Texture File in disk (mask) to unfoggedData array of bools

	if (GetIsTextureFileEnabled()) {

		if (!TextureInFile) {

			UE_LOG(LogTemp, Error, TEXT("Missing texture in LevelInfo, please load the mask!"));
			return;
		}
		if (TextureInFile != nullptr) {

			int TextureInFileSize = TextureInFile->GetSizeX();
			TextureInFileData.Init(FColor(1, 1, 1, 255), TextureInFileSize * TextureInFileSize);
			//init TArray
			//TextureInFileData.Init(FColor(0, 0, 0, 255), arraySize * arraySize);//init TArray, use this in unified version
			//The operation from Texture File ->to-> TArray of FColors

			UE_LOG(LogTemp, Warning, TEXT("Texture in file is :  %d  pixels wide"), TextureInFileSize);

			//Force texture compression to vectorDispl , https://wiki.unrealengine.com/Procedural_Materials

			//TODO here you need to add a halt or a warning to prevent the loading of textures that don´t meet the criteria
			//no mipmaps, compression to vector Displacement
			//i could check the compression settings, but they are in enum form

			//template<class TEnum>
			//class TEnumAsByte TextureCompressionStatus = TextureInFile->CompressionSettings;



			FTexture2DMipMap& Mip = TextureInFile->PlatformData->Mips[0];
			void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
			uint8* raw = NULL;
			raw = (uint8*)Data;
			FColor pixel = FColor(0, 0, 0, 255);//used for spliting the data stored in raw form

			for (int y = 0; y < TextureInFileSize; y++) {
				//data in the raw var is serialized i think ;)
				//so a pixel is four consecutive numbers e.g 0,0,0,255
				//and the following code split the values in single components and store them in a FColor
				for (int x = 0; x < TextureInFileSize; x++) {
					pixel.B = raw[4 * (TextureInFileSize * y + x) + 0];
					pixel.G = raw[4 * (TextureInFileSize * y + x) + 1];
					pixel.R = raw[4 * (TextureInFileSize * y + x) + 2];
					TextureInFileData[x + y * TextureInFileSize] = FColor((uint8)pixel.R, (uint8)pixel.G, (uint8)pixel.B, 255);

					//ToDo: IMPORTANT, YOU NEED TO THINK WHAT HAPPENS IF THE TEXTURE IN FILE HAS A DIFFERRENT SIZE THAN BOOL UNFOGGEDDATA, THIS IS COULD
					//CAUSE AN OUT OF BOUNDS ARRAY, OR SOMETHING

					//Here we are writing to the UnfoggedData Array the values that are already unveiled, from the texture file
					if (pixel.B >= 100) {
						UnfoggedData[x + y * TextureInFileSize] = true;
					}

				}
				//FColor Colorcito = TextureInFileData[y ];
				//UE_LOG(LogTemp, Warning, TEXT("TArray in y:  %d is :  %s"), y , *Colorcito.ToString());
			}

			//FColor Colorcito = TextureInFileData[524288];
			//UE_LOG(LogTemp, Warning, TEXT("TArray in 524288 is :  %s"), *Colorcito.ToString());

			Mip.BulkData.Unlock();
			TextureInFile->UpdateResource();
		}


		if (FOWTexture) {

			TextureData = TextureInFileData;
			LastFrameTextureData = TextureInFileData;
			//Missing the TArray<bool> for unfogged data, do this
		}

	}


}

void AFogOfWarManager::OnFowTextureUpdated_Implementation(UTexture2D* currentTexture, UTexture2D* lastTexture) {
	//Handle in blueprint
}

/*My Edits*/

void AFogOfWarManager::debugTextureAccess() {


}

void AFogOfWarManager::RegisterFowActor(AActor* Actor) {
	FowActors.Add(Actor);
}

bool AFogOfWarManager::GetIsBlurEnabled() {
	return bIsBlurEnabled;
}

bool AFogOfWarManager::GetIsTextureFileEnabled() {
	return bUseTextureFile;
}

void AFogOfWarManager::LogNames() {
	//Iterate over FowActors TArray
	for (auto& Actor : FowActors) {

		FString Name = Actor->GetName();
		UE_LOG(LogTemp, Warning, TEXT("The name of this actor is: %s"), *Name);
		Name = Actor->FindComponentByClass<URegisterToFOW>()->GetName();
		UE_LOG(LogTemp, Warning, TEXT("And the actor has a component: %s"), *Name);
		bool temp = Actor->FindComponentByClass<URegisterToFOW>()->WriteTerraIncog;
		if (temp) {
			UE_LOG(LogTemp, Warning, TEXT("can write Terra Incognita: TRUE"));
		}
		else {
			UE_LOG(LogTemp, Warning, TEXT("can write Terra Incognita: FALSE"));
		}


	}

	//FString TempString = GetName();
}

void AFogOfWarManager::UpdateTextureRegions(UTexture2D* Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D* Regions, uint32 SrcPitch, uint32 SrcBpp, uint8* SrcData, bool bFreeData)
{
	if (Texture && Texture->Resource)
	{
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			uint32 NumRegions;
			FUpdateTextureRegion2D* Regions;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
		};

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
		RegionData->MipIndex = MipIndex;
		RegionData->NumRegions = NumRegions;
		RegionData->Regions = Regions;
		RegionData->SrcPitch = SrcPitch;
		RegionData->SrcBpp = SrcBpp;
		RegionData->SrcData = SrcData;

		ENQUEUE_RENDER_COMMAND(FUpdateTextureRegionsData)(
			[RegionData = RegionData, bFreeData = bFreeData](FRHICommandListImmediate& RHICmdList)

		{
			for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
			{
				int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
				if (RegionData->MipIndex >= CurrentFirstMip)
				{
					RHIUpdateTexture2D(
						RegionData->Texture2DResource->GetTexture2DRHI(),
						RegionData->MipIndex - CurrentFirstMip,
						RegionData->Regions[RegionIndex],
						RegionData->SrcPitch,
						RegionData->SrcData
						+ RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
						+ RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
					);
				}
			}
			if (bFreeData)
			{
				FMemory::Free(RegionData->Regions);
				FMemory::Free(RegionData->SrcData);
			}
			delete RegionData;
		});
	}
}