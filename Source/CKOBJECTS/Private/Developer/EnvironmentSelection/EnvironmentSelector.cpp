// Fill out your copyright notice in the Description page of Project Settings.

#include "Developer/EnvironmentSelection/EnvironmentSelector.h"


void UEnvironmentSelector::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UEnvironmentSelector::Deinitialize()
{
	Super::Deinitialize();
}

void UEnvironmentSelector::SetCurrentEnvironment(const FEnvironmentDefinition& Environment)
{
	CurrentEnvironment = Environment;
	UE_LOG(LogTemp, Log, TEXT("Current Environment:%d"), CurrentEnvironment.Environment);
}

FString UEnvironmentSelector::GetGraphQLEndpoint()
{
	return CurrentEnvironment.GraphQl_Endpoint;
}

FString UEnvironmentSelector::GetCDNEndpoint()
{
	return CurrentEnvironment.CDN_Endpoint;
}

FString UEnvironmentSelector::GetUDPEndpoint()
{
	return CurrentEnvironment.UDP_Endpoint;
}

FString UEnvironmentSelector::GetWebAppEndpoint()
{
	return CurrentEnvironment.WebApp_Endpoint;
}

void UEnvironmentSelector::SetUDPEndPoint(const FString& InUDPEndpoint)
{
	CurrentEnvironment.UDP_Endpoint = InUDPEndpoint;
}
