// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Interfaces/IHttpRequest.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Engine/Engine.h"
#include "Http.h"
#include "Network//GraphQL/GraphQLQueryDatabase.h"
#include "Shared/Types/Interfaces/Subsystems/SubsystemInitializable.h"
#include "Shared/Types/Structures/Versioning/FGameVersion.h"
#include "GraphQLService.generated.h"

class AAvatarDataManager;
class UUDPSubsystem;
class UQueryMessageParser;
class UUserServiceSubsystem;
class UUserStateServiceSubsystem;
class UChunkServiceSubsystem;
class UVoxelServiceSubsystem;

DECLARE_LOG_CATEGORY_EXTERN(LogGraphQLService, Log, All);

DECLARE_DYNAMIC_DELEGATE_TwoParams(FOnGraphQLResponse, bool, bSuccess, const FString&, ResponseJson);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeleportResponse, bool, bTeleportAllowed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGetVersionInfo, FGameVersion, ServerVersion, FGameVersion, ClientVersion);

USTRUCT(BlueprintType)
struct CROWDEDKINGDOMS_API FGraphQLStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	int32 QueriesSentPerSecond;

	UPROPERTY(BlueprintReadOnly)
	int32 QueryBytesSentPerSecond;

	UPROPERTY(BlueprintReadOnly)
	int32 ResponseBytesReceivedPerSecond;

	UPROPERTY(BlueprintReadOnly)
	int32 ResponseReceivedPerSecond;

	UPROPERTY(BlueprintReadOnly)
	bool bIsReceivingData;
};


/**
 * @class UGraphQLService
 * @brief A subsystem allowing execution and management of GraphQL operations within the game instance.
 *
 * UGraphQLService serves as a dedicated system for interacting with GraphQL backends, enabling streamlined
 * communication with server-side APIs. It inherits from UGameInstanceSubsystem and provides specialized
 * handling for sending requests, managing context, and processing GraphQL responses.
 *
 * The class is designed to support features such as sending queries, executing mutations, and maintaining
 * the necessary state for seamless interaction with backend services in the context of the game application.
 */

UCLASS(BlueprintType)
class CROWDEDKINGDOMS_API UGraphQLService : public UGameInstanceSubsystem, public ISubsystemInitializable
{
	GENERATED_BODY()

public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;


	UFUNCTION(BlueprintCallable, Category = "GraphQL Service")
	virtual void PostSubsystemInit() override;
	
	/**
	 * @brief Sets the authentication token used for GraphQL requests.
	 *
	 * This method allows updating the token that is appended to authorized
	 * GraphQL requests. It ensures that the service can securely interact
	 * with a backend requiring token-based authentication.
	 *
	 * @param InAuthToken A string representing the authentication token to be set.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphQL Service")
	void SetAuthToken(const FString& InAuthToken);

	/**
	 * @brief Clears any stored authentication token for the GraphQL service.
	 *
	 * This method is used to reset the authentication state of the UGraphQLService by
	 * removing the currently stored authentication token. Upon clearing, no token will
	 * be included in subsequent authenticated requests unless a new token is set.
	 *
	 * This can be used, for example, when logging out a user or invalidating an existing session.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphQL Service")
	void ClearAuthToken();

	/**
	 * @brief Checks if an authentication token is available.
	 *
	 * This method determines whether a valid authentication token has been set in the service
	 * for use in authorized API requests. It returns true if the token is present and non-empty.
	 *
	 * @return True if an authentication token is available; otherwise, false.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphQL Service")
	bool HasAuthToken() const;

	

	/**
	 * Executes a predefined GraphQL query based on its ID, optionally including authentication
	 * tokens and runtime variables, and invokes the specified callback upon completion.
	 *
	 * This function retrieves the query template associated with the given query ID from the
	 * query database and merges its default variables with any provided runtime variables.
	 * The merged query is then executed, and the response is handled by the provided callback.
	 *
	 * @param QueryID The unique identifier for the predefined GraphQL query to be executed.
	 * @param RuntimeVariables A map of key-value pairs representing variables to be merged
	 *                          with the query's default variables during execution.
	 * @param bIncludeAuthToken A boolean flag indicating whether to include the authentication token
	 *                          in the GraphQL query. Set to true to include the token.
	 * @param bUseNestedJson
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphQL Service")
	void ExecuteQueryByID(EGraphQLQuery QueryID, const TMap<FString, FString>& RuntimeVariables, bool bIncludeAuthToken = true, bool
	                      bUseNestedJson = false);

	/**
	 * @brief Sets the endpoint URL for the GraphQL service.
	 *
	 * This method configures the endpoint that the GraphQL service will use for sending
	 * API requests. It is intended to define or update the address of the backend server,
	 * facilitating dynamic adjustments of the service's target endpoint during runtime.
	 *
	 * @param InEndpoint The new endpoint URL to be set for the service.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphQL")
	void SetEndpoint(const FString& InEndpoint);

	/**
	 * @brief Retrieves the current GraphQL endpoint URL based on the active environment.
	 *
	 * This method returns the appropriate GraphQL endpoint for the service depending on whether the
	 * application is operating in a development or production environment. It aids in ensuring that
	 * GraphQL queries are sent to the correct server based on the configured environment state.
	 *
	 * @return The current endpoint URL as a string; either the development or production endpoint.
	 */
	UFUNCTION(BlueprintCallable, Category = "GraphQL")
	FString GetCurrentEndpoint() const;

	/**
	 * @var UGraphQLService::GraphQLQueryDatabase
	 * @brief Represents a reference to the database containing predefined GraphQL queries.
	 *
	 * This variable holds a pointer to a UGraphQLQueryDatabase object, which defines and organizes
	 * a collection of GraphQL query definitions. It serves as the core repository for all
	 * query-related metadata and enables the UGraphQLService to retrieve and execute queries
	 * efficiently by their identifiers.
	 *
	 * The GraphQLQueryDatabase is critical for managing the pre-defined queries used throughout
	 * the system, allowing for clean abstraction and separation of query definitions from
	 * execution logic.
	 *
	 * @note Ensure this property is properly initialized, as any attempt to perform a query
	 * without a valid database reference will result in an error.
	 */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "GraphQL")
	UGraphQLQueryDatabase* GraphQLQueryDatabase;

	FOnGraphQLResponse OnComplete;

	UPROPERTY(BlueprintAssignable, Category = "GraphQL Service")
	FOnTeleportResponse OnTeleport;
	
	UFUNCTION(BlueprintCallable, Category = "GraphQL Service")
	void SetAvatarDataManager(AAvatarDataManager* InAvatarManager)
	{
		AvatarDataManager = InAvatarManager;
	}

	UFUNCTION(BlueprintCallable, Category = "GraphQL Service")
	FGraphQLStats GetStats() const;

	UPROPERTY(BlueprintAssignable, Category = "GraphQL Service")
	FOnGetVersionInfo OnGetVersionInfo;

	
private:

	/**
	 * @brief Executes a GraphQL query by sending an HTTP POST request to the configured GraphQL endpoint.
	 *
	 * This method handles the construction of the HTTP request, including authorization headers and payload serialization.
	 * It supports optional query variables and invokes a callback upon completion with the response details.
	 *
	 * @param Query The GraphQL query string to execute. This should be a valid GraphQL query or mutation.
	 * @param bIncludeAuthToken A boolean indicating whether to include an authorization token in the request headers.
	 * @param Variables An optional map of GraphQL variables to be included with the query.
	 *        If no variables are provided, this can be null.
	 * This is only called Internally
	 */
	void ExecuteGraphQLQuery(const FString& Query, const bool bIncludeAuthToken, const TSharedPtr<FJsonObject>& Variables);
	
	/**
	 * @brief Callback method for handling the completion of an HTTPS request.
	 *
	 * This method is invoked automatically when an HTTP request to the GraphQL backend is completed.
	 * It processes the server's response, logs relevant information, and invokes the provided callback
	 * with the result of the request.
	 *
	 * @param Request The HTTP request object associated with the GraphQL query.
	 * @param Response The HTTP response object returned by the server.
	 * @param bWasSuccessful A flag indicating whether the HTTP request was successful or not.
	 */
	void OnHttpsRequestComplete(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful);

	/**
	 * @brief Parses and routes a GraphQL backend response to the appropriate services for processing.
	 *
	 * This function consumes a GraphQL response in the form of JSON-formatted text, parses it, and determines
	 * the proper response type for dispatching to relevant subsystems or handlers within the game instance.
	 *
	 * If a message parser is not already initialized, it retrieves an instance of UQueryMessageParser
	 * from the game instance's subsystems to handle the response parsing.
	 *
	 * The response is classified into types such as login, register, get chunk, update chunk, or various
	 * avatar-related operations. Each type is logged and processed accordingly to ensure that the response
	 * is handled in a manner specific to its purpose. An error response type is managed for cases where
	 * parsing fails or other issues arise.
	 *
	 * @param ResponseContent A JSON-formatted string containing the raw response from the GraphQL server.
	 */
	void ParseAndDispatchToServices(const FString& ResponseContent) const;
	

	void HandleTeleportResponse(const TSharedPtr<FJsonObject>& Response) const;

	void HandleGetVersionInfo(const TSharedPtr<FJsonObject>& Response) const;

	UPROPERTY()
	UQueryMessageParser* MessageParser;

	UPROPERTY()
	FString AuthToken;

	UPROPERTY()
	bool bIsDevelopment;
	
	UPROPERTY()
	FString GraphQLEndpoint;

	// Services References
	UPROPERTY()
	UUserServiceSubsystem* UserService;

	UPROPERTY()
	UUserStateServiceSubsystem* UserStateService;

	UPROPERTY()
	UChunkServiceSubsystem* ChunkService;

	UPROPERTY()
	UVoxelServiceSubsystem* VoxelService;

	UPROPERTY()
	UUDPSubsystem* UDP_Service;

	UPROPERTY()
	AAvatarDataManager* AvatarDataManager;

	UFUNCTION()
	void UpdateStats();
	
	FThreadSafeCounter QueriesSentPerSecond;
	FThreadSafeCounter QueriesBytesSentPerSecond;
	FThreadSafeCounter ResponseBytesReceivedPerSecond;
	FThreadSafeCounter ResponseReceivedPerSecond;
	FThreadSafeBool bIsReceivingData;

	FTimerHandle StatsTimerHandle;
};
