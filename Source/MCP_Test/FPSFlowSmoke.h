#pragma once
class AFPSMatchMode;
/** Explicit opt-in runtime checks; never activated during ordinary play. */
struct FFPSFlowSmoke
{
    static void Start(AFPSMatchMode* Mode);
};
