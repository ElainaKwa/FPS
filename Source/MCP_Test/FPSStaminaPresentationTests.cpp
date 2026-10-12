#if WITH_DEV_AUTOMATION_TESTS
#include "FPSStaminaPresentation.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFPSStaminaSmoothingTest,"FPS.UI.StaminaSmoothing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FFPSStaminaSmoothingTest::RunTest(const FString& Parameters)
{
    FFPSStaminaPresentation S;
    S.Update(.7f,1.f/60.f);
    TestEqual(TEXT("First sample snaps, no spawn animation"),S.White(),.7f);
    S.Reset(1.f); S.Update(.4f,.08f);
    TestTrue(TEXT("Loss white responds while trail holds"),S.White()<1.f && S.Ghost()==1.f);
    S.Update(.4f,.1f);
    TestTrue(TEXT("Loss trail follows after hold"),S.Ghost()<1.f && S.Ghost()>S.White());
    for(int32 I=0;I<120;++I)S.Update(.4f,1.f/60.f);
    TestTrue(TEXT("Loss settles"),S.IsSettled());
    S.Update(.9f,.08f);
    TestTrue(TEXT("Gain gray leads, white holds"),S.Ghost()>.4f && S.White()==.4f);
    S.Update(.1f,.03f);
    TestTrue(TEXT("Reversal retains valid ordering"),S.White()>=.1f && S.Ghost()>=S.White() && S.Ghost()<=1.f);
    S.Reset(1.f);
    for(int32 I=1;I<=60;++I)S.Update(1.f-.5f*I/60.f,1.f/60.f);
    TestTrue(TEXT("Continuous drain does not hold trail forever"),S.Ghost()<.7f && S.White()>.49f);
    for(int32 I=0;I<150;++I)S.Update(1.f,1.f/60.f);
    TestTrue(TEXT("Full recovery eventually permits hiding"),S.IsSettled() && S.White()==1.f);
    S.Reset(.2f);S.Update(2.f,5.f);
    TestTrue(TEXT("Hitch is bounded and clamped"),S.White()==1.f && S.Ghost()==1.f);
    FFPSStaminaPresentation A,B;A.Reset(1);B.Reset(1);
    for(int32 I=0;I<15;++I)A.Update(.2f,1.f/30.f);
    for(int32 I=0;I<60;++I)B.Update(.2f,1.f/120.f);
    TestTrue(TEXT("Frame rate independent step response"),FMath::IsNearlyEqual(A.Ghost(),B.Ghost(),.0001f));
    S.Reset(.35f);
    TestTrue(TEXT("Respawn/reset clears old trail"),S.White()==.35f && S.IsSettled());
    return true;
}
#endif
