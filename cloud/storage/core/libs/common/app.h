#pragma once

class TProgramShouldContinue;

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

int AppMain(TProgramShouldContinue& shouldContinue);
void AppStop(int exitCode);

}   // namespace NCloud
