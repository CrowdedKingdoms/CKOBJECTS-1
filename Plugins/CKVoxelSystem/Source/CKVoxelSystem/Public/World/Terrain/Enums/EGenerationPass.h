#pragma once

UENUM()
enum class EGenerationPass : uint8
{
	Foundation,
	Carving,
	Surface,
	Decoration,
	Structures,
	Infrastructure,
	Details
};