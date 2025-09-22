#pragma once

UENUM()
enum EQueryResponseType: uint8
{
	Login,
	Register,
	UDP_Info,
	GetChunkByDistance,
	UpdateChunk,
	VoxelList,
	CreateAvatar,
	MyAvatars,
	UpdateAvatar,
	UpdateAvatarState,
	Error,
	ListVoxelUpdatesByDistance,
	TeleportRequest,
	DeleteAvatar,
	VersionInfo,
	UpdateUserState,
	GetUserState,
};
