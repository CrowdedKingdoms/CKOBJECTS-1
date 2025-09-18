// Fill out your copyright notice in the Description page of Project Settings.


#include "Shared/Types/Core/CKGameInstance.h"
#include "CKNetwork/Pubilc/Network/Services/User/UserStateServiceSubsystem.h"

#ifndef CK_VERSION_MAJOR
#define CK_VERSION_MAJOR 0
#endif
#ifndef CK_VERSION_MINOR
#define CK_VERSION_MINOR 0
#endif
#ifndef CK_VERSION_PATCH
#define CK_VERSION_PATCH 0
#endif
#ifndef CK_VERSION_BUILD
#define CK_VERSION_BUILD 0
#endif


void UCKGameInstance::Init()
{
	Super::Init();

	Major = CK_VERSION_MAJOR;
	Minor = CK_VERSION_MINOR;
	Patch = CK_VERSION_PATCH;
	Build = CK_VERSION_BUILD;
}

