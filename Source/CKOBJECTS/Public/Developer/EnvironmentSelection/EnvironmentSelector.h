// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentDefinition.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EnvironmentSelector.generated.h"

/**
 * Represents a utility class for managing and selecting environment configurations.
 *
 * UEnvironmentSelector provides mechanisms to define, update, and retrieve
 * environment settings, enabling seamless transitions between different
 * configurations during application execution.
 */
UCLASS(BlueprintType)
class  UEnvironmentSelector : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/**
	 * Sets the current environment configuration for the application.
	 *
	 * This function assigns the provided environment definition to the
	 * internal `CurrentEnvironment` member, applying the associated endpoint
	 * configurations and environment settings.
	 *
	 * @param Environment The environment definition that includes endpoint URLs
	 *                    and the environment type (e.g., Local, Development, Production).
	 */
	UFUNCTION(BlueprintCallable, Category = "Environment Selector")
	void SetCurrentEnvironment(const FEnvironmentDefinition& Environment);

	/**
	 * Retrieves the GraphQL API endpoint URL for the current environment.
	 *
	 * This method provides the URL string associated with the GraphQL API endpoint
	 * of the currently selected environment. The value is determined by the
	 * GraphQl_Endpoint property of the CurrentEnvironment object.
	 *
	 * @return The GraphQL API endpoint URL as a string.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Environment Selector")
	FString GetGraphQLEndpoint();

	/**
	 * Retrieves the CDN (Content Delivery Network) endpoint URL for the current environment.
	 *
	 * This method accesses the `CurrentEnvironment` and fetches the `CDN_Endpoint` property,
	 * which represents the URL used for CDN services in the selected environment.
	 *
	 * @return The CDN endpoint as a string for the currently configured environment.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Environment Selector")
	FString GetCDNEndpoint();

	/**
	 * Retrieves the UDP endpoint for the current environment.
	 *
	 * This function returns the endpoint address as a string
	 * that is configured for UDP communication in the current
	 * environment configuration.
	 *
	 * @return A string representing the UDP endpoint for the current environment.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Environment Selector")
	FString GetUDPEndpoint();

	/**
	 * Retrieves the web application endpoint URL from the current environment settings.
	 *
	 * This method fetches the URL string for the web application endpoint
	 * that is stored within the current environment configuration.
	 * The endpoint can be used for accessing web app-related services or resources.
	 *
	 * @return A string containing the web application endpoint URL for the current environment.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Environment Selector")
	FString GetWebAppEndpoint();


	UFUNCTION(BlueprintCallable, Category = "Environment Selector")
	void SetUDPEndPoint(const FString& InUDPEndpoint);
private:
	/**
	 * Stores the current environment configuration for the application.
	 *
	 * This variable holds an instance of FEnvironmentDefinition, which contains information about
	 * the environment being used, such as its type (e.g., Local, Development, Production) and
	 * its associated endpoint URLs (GraphQL, CDN, UDP, WebApp).
	 *
	 * The value of CurrentEnvironment is updated via the `SetCurrentEnvironment` function, allowing
	 * runtime changes to the application's environment configuration. The stored data is critical
	 * for selecting appropriate services and resources for the selected environment.
	 */
	FEnvironmentDefinition CurrentEnvironment;
	
};
