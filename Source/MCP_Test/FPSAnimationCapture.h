#pragma once
class AFPSDemoCharacter;
class UFPSCharacterPresentation;
class USceneComponent;
class UCameraComponent;
/** Opt-in real-world visual exercise, only active with -FPSAnimationCapture. */
void TickFPSAnimationCapture(AFPSDemoCharacter* Character, UFPSCharacterPresentation* Presentation,
    USceneComponent* Gun, UCameraComponent* Camera);
