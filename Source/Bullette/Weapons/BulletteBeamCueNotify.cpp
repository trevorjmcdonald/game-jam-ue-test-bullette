#include "Weapons/BulletteBeamCueNotify.h"

#include "GameFramework/Actor.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"

bool UBulletteBeamCueNotify::OnExecute_Implementation(AActor* MyTarget, const FGameplayCueParameters& Parameters) const
{
	if (!BeamSystem || !MyTarget || !Parameters.EffectContext.HasOrigin())
	{
		return false;
	}

	// Spawn inactive so BeamEnd is set before the first simulation tick; pooled because chains fire many at once.
	UNiagaraComponent* Beam = UNiagaraFunctionLibrary::SpawnSystemAtLocation(MyTarget, BeamSystem, Parameters.EffectContext.GetOrigin(),
		FRotator::ZeroRotator, FVector::OneVector, true, false, ENCPoolMethod::AutoRelease);
	if (Beam)
	{
		Beam->SetVariableVec3(TEXT("BeamEnd"), FVector(Parameters.Location));
		Beam->Activate(true);
	}
	return Beam != nullptr;
}
