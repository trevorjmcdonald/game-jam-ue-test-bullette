#include "Verify/JamKitChoice.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "JamKit.h"
#include "Verify/JamKitTrace.h"

UJamKitChoiceSubsystem* UJamKitChoiceSubsystem::Get(const UObject* WorldContext)
{
	const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr;
	return GameInstance ? GameInstance->GetSubsystem<UJamKitChoiceSubsystem>() : nullptr;
}

void UJamKitChoiceSubsystem::Offer(FName Id, TArray<FName> Options, TFunction<void(int32)> OnChosen)
{
	Open.RemoveAll([Id](const FJamKitChoice& Each) { return Each.Id == Id; });
	JamKitTrace::Event(GetGameInstance()->GetWorld(), TEXT("choice"), Id.ToString(), FJamKitTraceFields().Add(TEXT("options"), Options));
	Open.Add({Id, MoveTemp(Options), MoveTemp(OnChosen)});
}

void UJamKitChoiceSubsystem::Resolved(FName Id, FName Picked)
{
	if (Open.RemoveAll([Id](const FJamKitChoice& Each) { return Each.Id == Id; }) > 0 && !Picked.IsNone())
	{
		JamKitTrace::Event(GetGameInstance()->GetWorld(), TEXT("chose"), Id.ToString(), FJamKitTraceFields().Add(TEXT("picked"), Picked).Add(TEXT("by"), TEXT("player")));
	}
}

bool UJamKitChoiceSubsystem::Choose(FName Id, int32 Index, const FString& By)
{
	const int32 Found = Open.IndexOfByPredicate([Id](const FJamKitChoice& Each) { return Each.Id == Id; });
	if (Found == INDEX_NONE || !Open[Found].Options.IsValidIndex(Index))
	{
		return false;
	}
	// Out of the list first: what the choice does may offer the next one (another level up straight away).
	FJamKitChoice Choice = MoveTemp(Open[Found]);
	Open.RemoveAt(Found);
	JamKitTrace::Event(GetGameInstance()->GetWorld(), TEXT("chose"), Id.ToString(), FJamKitTraceFields().Add(TEXT("picked"), Choice.Options[Index]).Add(TEXT("by"), By));
	UE_LOG(LogJamKit, Display, TEXT("Choice: %s -> %s (%s)"), *Id.ToString(), *Choice.Options[Index].ToString(), *By);
	if (Choice.OnChosen)
	{
		Choice.OnChosen(Index);
	}
	return true;
}

const FJamKitChoice* UJamKitChoiceSubsystem::Find(FName Id) const
{
	return Open.FindByPredicate([Id](const FJamKitChoice& Each) { return Each.Id == Id; });
}

void JamKitChoice::Offer(const UObject* WorldContext, FName Id, TArray<FName> Options, TFunction<void(int32)> OnChosen)
{
	if (UJamKitChoiceSubsystem* Choices = UJamKitChoiceSubsystem::Get(WorldContext))
	{
		Choices->Offer(Id, MoveTemp(Options), MoveTemp(OnChosen));
	}
}

void JamKitChoice::Resolved(const UObject* WorldContext, FName Id, FName Picked)
{
	if (UJamKitChoiceSubsystem* Choices = UJamKitChoiceSubsystem::Get(WorldContext))
	{
		Choices->Resolved(Id, Picked);
	}
}
