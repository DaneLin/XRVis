#include "RenderPasses/ChartDataGenPass.h"

IMPLEMENT_SHADER_TYPE(, FAxisChartGenCS, TEXT("/XRVis/AxisChartDataGenCS.usf"), TEXT("GenBoxDataCS"), SF_Compute);