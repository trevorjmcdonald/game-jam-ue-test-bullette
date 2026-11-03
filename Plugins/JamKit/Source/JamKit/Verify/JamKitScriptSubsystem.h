#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "JamKitScriptSubsystem.generated.h"

class AActor;
class APawn;
class FJsonObject;
class UInputAction;

/**
 * Plays the game from a script, unattended, and checks what happens: launch with -jamscript=<path> (relative to the
 * project folder; or put the argument in Saved/jamkit-args.txt) and it runs the steps in order, through the real input
 * (Enhanced Input injection, so the game's own bindings, triggers and handlers run), then quits with exit code 0 if
 * every expectation held and 1 if not. Results go to Saved/Script/<name>/result.jsonl, shots beside it.
 *
 * A script: {"name": "...", "seed": 7, "timeout": 600, "report": "kind", "on": [handlers], "steps": [steps]}.
 * "seed": the gameplay random seed (UJamKitRandomSubsystem) unless -jamseed is given, so a script repeats exactly.
 * Each step is an object whose first known key says what it does:
 *   {"open": "/Game/Maps/L_X", "options": "Arrival=Y", "commands": ["..."], "settle": 3}   open a level, wait for the pawn
 *   {"exec": "console command"}
 *   {"teleport": [x, y, z], "yaw": 90, "pitch": -10}   the pawn (capsule centre) and, if given, the view
 *   {"look": [pitch, yaw]}                              set the view
 *   {"lookAt": [x, y, z] or "Actor"}                    turn the view to face it
 *   {"press": "IA_Interact", "value": [1, 0, 0]}        one press of an input action (by name in the Jam Kit settings'
 *                                                       RebindContext, or an object path)
 *   {"hold": "IA_Move", "value": [0, 1], "seconds": 2}  hold an action's value for a time
 *   {"moveTo": [x, y, z] or "Actor", "action": "IA_Move", "radius": 100, "within": 10, "frame": "view"}
 *        steer there with a 2D move action (X right, Y forward; "frame": "world" if the game maps it onto world X/Y)
 *   {"wait": 1.5}
 *   {"set": "Actor", "component": "Optional", "property": "Name", "value": "text as in a .ini"}
 *   {"call": "Actor", "function": "Name and args"}     a UFUNCTION with no or text arguments
 *        (Actor: an actor's name or class, exactly, else the first whose name or class contains it)
 *   {"choose": "ChoiceId", "pick": "Name" or 2, "prefer": ["A", "B"], "random": true, "within": 5}
 *        answer a choice the game offered (UJamKitChoiceSubsystem): the first option containing the earliest
 *        preference, else "pick", else a random one (the script's own seeded stream), else the first
 *   {"expect": "kind", "contains": "text", "within": 3, "not": false, "count": 1, "since": "action"}
 *        a check: a trace line of that kind whose "what" contains the text ("count": at least that many), since the
 *        last action step of the main steps began (open, exec, teleport, look, lookAt, press, hold, moveTo, set, call,
 *        choose) or, with "since": "start", in the whole run; waits up to "within" seconds
 *   {"waitFor": "kind", "contains": "text", "within": 60}   the same, as a sync point rather than a check
 *   {"repeat": [steps], "times": 3, "seconds": 60, "until": {"event": "kind", "contains": "text"}}
 *        run the steps again and again until any limit is met (until: checked between steps)
 *   {"shot": "name", "ui": true}                        save the screen (with the HUD unless "ui" is false) and its brightness
 *   {"note": "text"}                                    a line in the trace and the log
 * Handlers ("on") run whenever a trace line matches, wherever the main steps are, and the main steps carry on after:
 *   {"event": "choice", "contains": "LevelUp", "do": [{"choose": "LevelUp", "random": true}]}
 * "report": the kind of trace line that sums up a run (e.g. "gameover"); its last one goes in the results, and
 * tools/play-script.ps1 -Seeds tabulates it across seeds.
 * Steps run back to back; give the game time with "wait" where a real player would pause. Times are game-loop time:
 * with -jamfixedstep they run as fast as the machine can.
 */
UCLASS()
class JAMKIT_API UJamKitScriptSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

private:
	using FSteps = TArray<TSharedPtr<FJsonObject>>;

	struct FTraceLine
	{
		FName Kind;
		FString What;
		FString Line;
	};

	struct FHandler
	{
		FName Kind;
		FString Contains;
		FSteps Steps;
		int32 Pending = 0;
		bool bRunning = false;
	};

	/** A list of steps being run: the script's, a repeat's or a handler's. */
	struct FFrame
	{
		FSteps Steps;
		int32 Index = 0;
		bool bStarted = false;
		double StepTime = 0.0;
		double SettleTime = 0.0;
		TWeakObjectPtr<const UInputAction> HeldAction;
		// A repeat's limits.
		bool bLoop = false;
		int32 Times = 0;
		int32 Rounds = 0;
		double Seconds = 0.0;
		double Elapsed = 0.0;
		TSharedPtr<FJsonObject> Until;
		int32 LoopCursor = 0;
		// A handler's.
		int32 Handler = INDEX_NONE;
	};

	bool Tick(float DeltaTime);
	/** Starts the frame's current step; true when it's already done. */
	bool StartStep(FFrame& Frame, const FJsonObject& Step);
	/** Carries on with a step that takes time; true when it's done. */
	bool UpdateStep(FFrame& Frame, const FJsonObject& Step, float DeltaTime);
	void EndStep(const FFrame& Frame, const FJsonObject& Step, bool bOk, const FString& Note, const FString& MatchedLine = FString());
	void PopFrame();
	bool LoopIsOver(const FFrame& Frame) const;
	void Finish(const FString& Why = FString());

	UWorld* GetGameWorld() const;
	APlayerController* GetPlayer() const;
	const UInputAction* FindAction(const FString& Name) const;
	AActor* FindActor(const FString& Name) const;
	bool FindTarget(const FJsonObject& Step, const TCHAR* Field, FVector& OutLocation) const;
	bool Inject(FFrame& Frame, const FJsonObject& Step, bool bContinuous, FString& OutNote);
	void StopHold(FFrame& Frame);
	bool SetProperty(const FJsonObject& Step, FString& OutNote) const;
	/** The index of the first matching trace line from From on, or INDEX_NONE. */
	int32 FindLine(FName Kind, const FString& Contains, int32 From) const;
	bool TryChoose(const FJsonObject& Step, FString& OutNote);
	void HandleTraceLine(FName Kind, const FString& What, const FString& Line);
	void HandleScreenshot(int32 Width, int32 Height, const TArray<FColor>& Colors);
	void WriteResult(const FString& Line) const;

	FString Name;
	FString OutputDir;
	FString ReportKind;
	double Timeout = 600.0;
	double RunTime = 0.0;
	TArray<FFrame> Stack;
	TArray<FHandler> Handlers;
	bool bStarted = false;
	double StartTime = 0.0;
	double ExitTimer = -1.0;
	int32 Passed = 0;
	int32 Failed = 0;

	/** Trace lines since the script started, and where the last action step began among them. */
	TArray<FTraceLine> Lines;
	int32 ActionCursor = 0;

	/** The open step's level. */
	FString OpeningLevel;

	TArray<FColor> CapturedPixels;
	FIntPoint CapturedSize = FIntPoint::ZeroValue;
	bool bCapturing = false;
	bool bCaptured = false;

	FTSTicker::FDelegateHandle TickerHandle;
	FDelegateHandle ScreenshotHandle;
	FDelegateHandle TraceHandle;
};
