#pragma once

/**
 * @brief Represents the different server environments supported.
 *
 * This enum is used to define the current environment configuration
 * the application is running in, such as Local, Development, or
 * Production. Each environment has its own specific settings and
 * use cases.
 */
UENUM(BlueprintType)
enum class EEnvironment : uint8
{
	Local UMETA(DisplayName="Local Server Environment"),
	Development UMETA(DisplayName="Development Environment"),
	Production UMETA(DisplayName="Production Environment")
};