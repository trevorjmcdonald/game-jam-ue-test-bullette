#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "BulletteAttributeSet.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FBulletteOutOfHealthDelegate, AActor* /*Instigator*/);

/**
 * Health for anything that can be hurt (player and enemies). Single-player, so nothing replicates.
 * Damage is a meta attribute: damage effects add to it, and PostGameplayEffectExecute turns it into Health loss.
 */
UCLASS()
class BULLETTE_API UBulletteAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(UBulletteAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Health")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(UBulletteAttributeSet, MaxHealth)

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS_BASIC(UBulletteAttributeSet, Damage)

	/** Broadcast once, the first time Health reaches zero. */
	FBulletteOutOfHealthDelegate OnOutOfHealth;

protected:
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

private:
	bool bOutOfHealth = false;
};
