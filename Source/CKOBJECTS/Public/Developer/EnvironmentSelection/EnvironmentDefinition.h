#pragma once

#include "CoreMinimal.h"
#include "Environment.h"
#include "EnvironmentDefinition.generated.h"

/**
 * Represents the definition of an environment with various endpoints.
 *
 * This struct is used to define the configuration settings for an environment.
 * It contains the endpoints for GraphQL, CDN, UDP, and a web application.
 */
USTRUCT(BlueprintType)
struct FEnvironmentDefinition
{
	GENERATED_BODY()

	/**
	 * Represents the current environment type.
	 *
	 * This property defines the environment (e.g., Local, Development, Production)
	 * in which the application is running. It is an enumerator that allows selection
	 * of the desired environment and can be accessed and modified within Blueprints.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EEnvironment Environment;
	
	/**
	 * Holds the URL string for the GraphQL API endpoint.
	 * Can be accessed and modified in Blueprints.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString GraphQl_Endpoint;

	/**
	 * Represents the endpoint URL for the CDN (Content Delivery Network).
	 * This property can be read and modified in both C++ and Blueprint.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString CDN_Endpoint;

	/**
	 * Represents the UDP endpoint address as a string, which can be used
	 * for communication or data transfer. This property is exposed to
	 * Blueprints for read and write access.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString UDP_Endpoint;

	/**
	 * The endpoint URL for the web application configuration.
	 * It stores the address as a string and can be read or modified in both
	 * Blueprint and C++ code.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString WebApp_Endpoint;
};
