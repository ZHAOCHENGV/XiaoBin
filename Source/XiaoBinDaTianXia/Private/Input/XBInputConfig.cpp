// Copyright XiaoBing Project. All Rights Reserved.

#include "Input/XBInputConfig.h"
#include "InputAction.h"

const UInputAction* UXBInputConfig::FindInputActionByTag(const FGameplayTag& InputTag) const
{
	for (const FXBInputActionMapping& Mapping : ExtendedInputMappings)
	{
		if (Mapping.InputTag == InputTag && Mapping.InputAction)
		{
			return Mapping.InputAction;
		}
	}
	return nullptr;
}

FGameplayTag UXBInputConfig::FindTagByInputAction(const UInputAction* InputAction) const
{
	for (const FXBInputActionMapping& Mapping : ExtendedInputMappings)
	{
		if (Mapping.InputAction == InputAction)
		{
			return Mapping.InputTag;
		}
	}
	return FGameplayTag();
}