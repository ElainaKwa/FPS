#pragma once

class AFPSFrontController;

/** Opt-in, same-process OpenLevel/input injection; never enables seamless travel. */
struct FFPSCharacterTravelSmoke
{
    static void Tick(AFPSFrontController* Controller);
};
